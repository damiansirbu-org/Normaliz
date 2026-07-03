#!/usr/bin/env bash

## script for the installation of e-antic for the use in libnormaliz

set -e

echo "::group::e-antic"

source $(dirname "$0")/common.sh

if [ "$GMP_INSTALLDIR" != "" ]; then
    export CPPFLAGS="${CPPFLAGS} -I${GMP_INSTALLDIR}/include"
    export LDFLAGS="${LDFLAGS} -L${GMP_INSTALLDIR}/lib"
fi

if [ "$OSTYPE" == "msys" ]; then
	echo "Hiding libmpfr.la and libflint.a"
	mkdir -p ${PREFIX}/lib/hide
	## mv -f ${PREFIX}/lib/libmpfr.la ${PREFIX}/lib/hide
	## mv -f ${PREFIX}/lib/libflint.a ${PREFIX}/lib/hide
fi

## E_ANTIC_VERSION=1.2.1
E_ANTIC_VERSION=2.0.2
E_ANTIC_URL="https://github.com/flatsurf/e-antic/releases/download/${E_ANTIC_VERSION}/e-antic-${E_ANTIC_VERSION}.tar.gz"
## E_ANTIC_SHA256=a7bfb92620fd7e42a06efbe89e011abee88f4fbd99bcec34fd8300ae9b1cf543
E_ANTIC_SHA256=8328e6490129dfec7f4aa478ebd54dc07686bd5e5e7f5f30dcf20c0f11b67f60

CONFIGURE_FLAGS="${CONFIGURE_FLAGS} --prefix=${PREFIX} --disable-silent-rules --without-byexample --without-doc --without-benchmark --without-pyeantic"

# MSYS2 automake cannot bootstrap the dependency-tracking makefile fragments;
# disable it (harmless for a one-shot build).
if [ "$OSTYPE" == "msys" ]; then
	CONFIGURE_FLAGS="${CONFIGURE_FLAGS} --disable-dependency-tracking"
fi

echo "Installing E-ANTIC..."

mkdir -p ${NMZ_OPT_DIR}/E-ANTIC_source/
cd ${NMZ_OPT_DIR}/E-ANTIC_source

../../download.sh ${E_ANTIC_URL} ${E_ANTIC_SHA256}
if [ ! -d e-antic-${E_ANTIC_VERSION} ]; then
    tar -xvf e-antic-${E_ANTIC_VERSION}.tar.gz
	cd e-antic-${E_ANTIC_VERSION}/libeantic
	sed -i -e s/fmpq_poly_add_fmpq/fmpq_poly_add_fmpq_eantic/g upstream/patched/fmpq_poly_add_fmpq.c
    cp ../../../../install_scripts_opt/e-antic_patches/nf_elem_add_fmpq.c upstream/patched/
	# FLINT (>=3.0) also defines fmpz_poly_randtest_irreducible in libflint.a;
	# rename e-antic's own copy to avoid a multiple-definition link error on
	# Windows (mingw ld is strict). Reproduces the removed randtest patch.
	if [ "$OSTYPE" == "msys" ]; then
		for f in $(grep -rl fmpz_poly_randtest_irreducible .); do
			sed -i s/fmpz_poly_randtest_irreducible/fmpz_poly_randtest_irreducible_eantic/g "$f"
		done
	fi
	cd ../..
fi

cd e-antic-${E_ANTIC_VERSION}/libeantic


if [ ! -f config.status ]; then
    ./configure ${CONFIGURE_FLAGS}
fi
make -j4
make install

if [ "$OSTYPE" == "msys" ]; then
	# The corresponding "hide" moves above are commented out, so hide/ is empty;
	# only restore if something is actually there (avoids a set -e abort).
	if ls ${PREFIX}/lib/hide/* >/dev/null 2>&1; then
		echo "Restoring libmpfr.la and libflint.a"
		cp ${PREFIX}/lib/hide/* ${PREFIX}/lib
	fi
fi
