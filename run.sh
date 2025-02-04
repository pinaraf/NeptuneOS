ARCH=i386
BUILD_TYPE=Debug

if [[ ${1,,} == "release" || ${2,,} == "release" ]]; then
    BUILD_TYPE=release
fi

if [[ $1 == "amd64" || $2 == "amd64" ]]; then
    ARCH=amd64
fi

BUILDDIR="build-$ARCH-${BUILD_TYPE,,}"
IMAGEDIR="images-$ARCH-${BUILD_TYPE,,}"

cd "$(dirname "$0")"

if [[ ARCH == "i386" ]]; then
    QEMU="qemu-system-i386  -cpu pentium3,-vme,-pdpe1gb,-xsave,-xsaveopt,-xsavec,-fsgsbase,-invpcid,enforce"
else
    QEMU="qemu-system-x86_64  -cpu Nehalem,+fsgsbase,-pdpe1gb"
fi

declare -a ARGS
for var in "$@"; do
    # Ignore known bad arguments
    if [ "${var,,}" == 'release' ]; then
        continue
    fi
    if [ "${var,,}" == 'debug' ]; then
        continue
    fi
    if [ "${var,,}" == 'amd64' ]; then
        continue
    fi
    if [ "${var,,}" == 'i386' ]; then
        continue
    fi
    ARGS[${#ARGS[@]}]="$var"
done

$QEMU -m size=400M -soundhw all -serial stdio -kernel $BUILDDIR/$IMAGEDIR/kernel -initrd $BUILDDIR/$IMAGEDIR/ntos "${ARGS[@]}"
