#include "obp.h"

static inline BOOLEAN NeedReparse(IN NTSTATUS Status)
{
    return (Status == STATUS_REPARSE) || (Status == STATUS_REPARSE_OBJECT);
}

/*
 * Parse the object path by recursively invoke the parse procedures
 * and return the final object to which the path points, in which
 * case the remaining path is empty, or until either the object does
 * not have a parse procedure or if the parse procedure returns
 * STATUS_NTOS_INVOKE_OPEN_ROUTINE. The remaining path points to
 * the sub-path that is yet to be parsed.
 *
 * If the parse procedure returns error, the resulting object of the
 * last successful parse is returned, as well as the remaining path
 * that is yet to be parsed.
 *
 * If at any point the parse procedure returned STATUS_REPARSE or
 * STATUS_REPARSE_OBJECT, the remaining path is set to the full path
 * of the new path to be parsed, and the object manager restarts the
 * new parse from the very beginning.
 */
NTSTATUS ObpLookupObjectName(IN PCSTR Path,
			     IN POB_PARSE_CONTEXT ParseContext,
			     OUT PCSTR *pRemainingPath,
			     OUT POBJECT *FoundObject)
{
    assert(ObpRootObjectDirectory != NULL);
    assert(Path != NULL);
    assert(FoundObject != NULL);

    /* We always start the lookup from the root directory.
     * It is an error to specify a relative path. */
    if (Path[0] != OBJ_NAME_PATH_SEPARATOR) {
	return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }

    /* Points to the terminating '\0' charactor of Path */
    PCSTR LastByte = Path + strlen(Path);

    /* Skip the leading OBJ_NAME_PATH_SEPARATOR. */
    Path++;

    /* Recursively invoke the parse procedure of the object, until
     * one of the following is true:
     *   1. The remaining path is empty
     *   2. The parse procedure returns a failure status
     *   3. The parse procedure is NULL
     *   4. The parse procedure returns STATUS_NTOS_INVOKE_OPEN_ROUTINE
     *   5. The parse procedure returns STATUS_REPARSE or STATUS_REPARSE_OBJECT
     */
    POBJECT Object = ObpRootObjectDirectory;
    NTSTATUS Status = STATUS_SUCCESS;

    while (*Path != '\0') {
	POBJECT_HEADER ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
	OBJECT_PARSE_METHOD ParseProc = ObjectHeader->Type->TypeInfo.ParseProc;
	if (ParseProc == NULL) {
	    break;
	}
	POBJECT Subobject = NULL;
	PCSTR RemainingPath = NULL;
	Status = ParseProc(Object, Path, ParseContext, &Subobject, &RemainingPath);
	if (!NT_SUCCESS(Status) || NeedReparse(Status) ||
	    Status == STATUS_NTOS_INVOKE_OPEN_ROUTINE) {
	    break;
	}
	/* Parse procedure should not return a NULL Subobject (unless reparsing) */
	assert(Subobject != NULL);
	/* Remaining path should also be within the original Path (unless reparsing) */
	assert(RemainingPath > Path);
	assert(RemainingPath <= LastByte);
	/* Skip the leading OBJ_NAME_PATH_SEPARATOR and keep going */
	if (*RemainingPath == OBJ_NAME_PATH_SEPARATOR) {
	    RemainingPath++;
	}
	Object = Subobject;
	Path = RemainingPath;
    }

    *pRemainingPath = Path;
    *FoundObject = Object;
    return Status;
}

/*
 * Open the given object by first invoking the parse procedures and then
 * invoking the open procedures, the latter of which can sleep. Assign
 * handle to the process if the open procedure has consumed all of specified
 * object path (ie. RemainingPath is empty).
 *
 * Type specifies the object type of the handle (ie. specify OBJECT_TYPE_FILE
 * if you are opening a DEVICE object).
 */
NTSTATUS ObOpenObjectByName(IN ASYNC_STATE AsyncState,
			    IN PTHREAD Thread,
			    IN OB_OBJECT_ATTRIBUTES ObjectAttributes,
			    IN OBJECT_TYPE_ENUM Type,
			    IN POB_PARSE_CONTEXT ParseContext,
			    OUT HANDLE *pHandle)
{
    assert(ObpRootObjectDirectory != NULL);
    assert(Thread != NULL);
    assert(Thread->Process != NULL);
    assert(pHandle != NULL);

    NTSTATUS Status = STATUS_NTOS_BUG;
    POBJECT OpenedInstance = NULL;
    PCSTR RemainingPath = NULL;

    ASYNC_BEGIN(AsyncState, Locals, {
	    POBJECT Object;
	    POBJECT UserRootDirectory;
	    PCSTR Path;
	    BOOLEAN Reparsed;
	});

    if (ObjectAttributes.ObjectNameBuffer == NULL && ObjectAttributes.RootDirectory == NULL) {
	ASYNC_RETURN(AsyncState, STATUS_OBJECT_NAME_INVALID);
    }

    Locals.Object = ObpRootObjectDirectory;
    Locals.UserRootDirectory = NULL;
    Locals.Path = ObjectAttributes.ObjectNameBuffer ? ObjectAttributes.ObjectNameBuffer : "";
    Locals.Reparsed = FALSE;	/* TRUE if Path has been set to a newly allocated string */

    /* If caller has specified a root directory, the object path must be relative */
    if (ObjectAttributes.RootDirectory != NULL && Locals.Path[0] == OBJ_NAME_PATH_SEPARATOR) {
	ASYNC_RETURN(AsyncState, STATUS_OBJECT_NAME_INVALID);
    }
    /* Get the root directory if user has specified one. Otherwise, default to the
     * global root object directory. */
    if (ObjectAttributes.RootDirectory != NULL) {
	ASYNC_RET_ERR(AsyncState, ObReferenceObjectByHandle(Thread->Process,
							    ObjectAttributes.RootDirectory,
							    OBJECT_TYPE_ANY,
							    &Locals.UserRootDirectory));
	assert(Locals.UserRootDirectory != NULL);
	Locals.Object = Locals.UserRootDirectory;
    }
    /* Skip the leading OBJ_NAME_PATH_SEPARATOR. */
    if (Locals.Path[0] == OBJ_NAME_PATH_SEPARATOR) {
	Locals.Path++;
    }

parse:
    /* Invoke the parse procedure first. If there isn't a parse procedure,
     * invoke the open procedure instead */
    if (OBJECT_TO_OBJECT_HEADER(Locals.Object)->Type->TypeInfo.ParseProc == NULL) {
	goto open;
    }
    /* If the path to be parsed is now empty, we are done parsing. Open the object. */
    if (*(Locals.Path) == '\0') {
	goto open;
    }
    POBJECT Subobject = NULL;
    Status = OBJECT_TO_OBJECT_HEADER(Locals.Object)->Type->TypeInfo.ParseProc(
	Locals.Object, Locals.Path, ParseContext, &Subobject, &RemainingPath);
    if (!NT_SUCCESS(Status)) {
	DbgTrace("Parsing path %s failed\n", Locals.Path);
	goto out;
    }
    /* If the parse routine indicates reparse, rerun the parse routine with the
     * new path utill it finds the actual object */
    if (NeedReparse(Status)) {
	/* If Path has been set to a newly allocated string due to a previous
	 * reparse, we need to free it since we are overwriting the Path pointer
	 * below with the ptr to the new string (allocated by the parse routine). */
	if (Locals.Reparsed) {
	    ExFreePool(Locals.Path);
	}
	/* The RemainingPath is now the new string to be parsed. It is allocated
	 * by the parse routine. We must free it ourselves later. */
	Locals.Path = RemainingPath;
	/* If we have an absolute path, reparse should start from the global root directory */
	if (Locals.Path[0] == OBJ_NAME_PATH_SEPARATOR) {
	    Locals.Object = ObpRootObjectDirectory;
	    /* Skip the leading OBJ_NAME_PATH_SEPARATOR. */
	    Locals.Path++;
	}
	/* Otherwise, for a relative path reparse should start from the current object.
	 * In this case we do not need to do anything. */
	DbgTrace("Reparsing with new path %s\n", Locals.Path);
	/* Set Reparsed to TRUE so that we remember to free the new string */
	Locals.Reparsed = TRUE;
	goto parse;
    }
    /* If we get here, either the parse routine has successfully parsed (part
     * of) the sub-path and returned a sub-object, or that it has indicated that
     * we need to invoke the open routine. In the previous case, we move forward
     * by invoking the parse routine on the sub-object. This is recursive, until
     * the remaining path is empty. */
    if (Status != STATUS_NTOS_INVOKE_OPEN_ROUTINE) {
	Locals.Object = Subobject;
	if (*RemainingPath == OBJ_NAME_PATH_SEPARATOR) {
	    RemainingPath++;
	}
	Locals.Path = RemainingPath;
	DbgTrace("Parsing sub-path %s\n", Locals.Path);
	goto parse;
    }
    /* If the parse routine has indicated that we must invoke the open routine
     * to fully parse a sub-path, it should set the remaining path to the full
     * sub-path and set the subobject to NULL. */
    assert(Locals.Path == RemainingPath);
    assert(Subobject == NULL);

open:
    assert(Locals.Object != NULL);
    assert(OBJECT_TO_OBJECT_HEADER(Locals.Object)->Type != NULL);
    /* If the object type does not define an open procedure, return error */
    if (OBJECT_TO_OBJECT_HEADER(Locals.Object)->Type->TypeInfo.OpenProc == NULL) {
	DbgTrace("Type %s does not have an open procedure\n",
		 OBJECT_TO_OBJECT_HEADER(Locals.Object)->Type->Name);
	Status = STATUS_OBJECT_NAME_INVALID;
	goto out;
    }

    /* Increase the reference count of the object so it doesn't get deleted. */
    ObpReferenceObject(Locals.Object);

    /* Call the open procedure, and wait asynchronously. */
    AWAIT_EX(Status,
	     OBJECT_TO_OBJECT_HEADER(Locals.Object)->Type->TypeInfo.OpenProc,
	     AsyncState, Locals, Thread, Locals.Object, Locals.Path,
	     ParseContext, &OpenedInstance, &RemainingPath);

    if (!NT_SUCCESS(Status)) {
	DbgTrace("Opening subpath %s failed\n", Locals.Path);
	ObDereferenceObject(Locals.Object);
	goto out;
    }
    /* If the open routine did not return error, it should always set the remaining path */
    assert(RemainingPath != NULL);

    /* If the open procedure returned REPARSE, start from the very beginning */
    if (NeedReparse(Status)) {
	/* If reparsing, the open procedure should never touch OpenedInstance */
	assert(OpenedInstance == NULL);
	/* If the saved path has been previously set to a new reparse string,
	 * we should free it since we will no longer refer to the old saved path */
	if (Locals.Reparsed) {
	    ExFreePool(Locals.Path);
	}
	Locals.Path = RemainingPath;
	/* If we have an absolute path, reparse should start from the global root directory */
	if (Locals.Path[0] == OBJ_NAME_PATH_SEPARATOR) {
	    Locals.Object = ObpRootObjectDirectory;
	    /* Skip the leading OBJ_NAME_PATH_SEPARATOR. */
	    Locals.Path++;
	    /* Decrease the reference count of the object since we don't refer to it anymore */
	    ObDereferenceObject(Locals.Object);
	}
	/* Otherwise, for a relative path reparse should start from the current object.
	 * In this case we do not need to do anything. */
	Locals.Reparsed = TRUE;
	DbgTrace("Reparsing with new path %s\n", Locals.Path);
	goto parse;
    }

    /* Otherwise, the OpenedInstance should not be NULL */
    assert(OpenedInstance != NULL);
    Locals.Object = OpenedInstance;
    /* Increase the reference count since we now exposing it to client space */
    ObpReferenceObject(Locals.Object);
    /* If RemainingPath is not empty, invoke the parse routine of the newly
     * opened instance with the remaining path, until the remaining path is empty */
    if (*RemainingPath != '\0') {
	if (*RemainingPath == OBJ_NAME_PATH_SEPARATOR) {
	    RemainingPath++;
	}
	Locals.Path = RemainingPath;
	goto parse;
    }

    /* RemainingPath is empty. Open is successful. Check the object type. */
    if (Type == OBJECT_TYPE_ANY || Type == OBJECT_TO_OBJECT_HEADER(Locals.Object)->Type->Index) {
	/* Type is valid. Assign the handle. */
	Status = ObCreateHandle(Thread->Process, Locals.Object, pHandle);
    } else {
	Status = STATUS_OBJECT_TYPE_MISMATCH;
    }

    if (!NT_SUCCESS(Status)) {
	/* TODO: Invoke close procedure of opened instance */
	ObDereferenceObject(Locals.Object);
    }
    Status = STATUS_SUCCESS;

out:
    if (Locals.Reparsed) {
	ExFreePool(Locals.Path);
    }
    if (Locals.UserRootDirectory != NULL) {
	ObDereferenceObject(Locals.UserRootDirectory);
    }
    ASYNC_END(AsyncState, Status);
}
