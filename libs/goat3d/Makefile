!ifdef __UNIX__
obj = src/aabox.obj src/extmesh.obj src/goat3d.obj src/track.obj src/chunk.obj &
	src/g3danm.obj src/log.obj src/write.obj src/dynarr.obj src/g3dscn.obj &
	src/read.obj src/readgltf.obj src/json.obj src/util.obj

incdir = -Iinclude -I.. -I../../src
alib = ../../goat3d.lib
!else
obj = src\aabox.obj src\extmesh.obj src\goat3d.obj src\track.obj src\chunk.obj &
	src\g3danm.obj src\log.obj src\write.obj src\dynarr.obj src\g3dscn.obj &
	src\read.obj src\readgltf.obj src\json.obj src\util.obj

incdir = -Iinclude -I.. -I..\..\src
alib = ..\..\goat3d.lib
!endif

!include ../../watcfg.mk
def = -dM_PI=3.141592653589793

CC = wcc386
CFLAGS = $(cfg_dbg) $(cfg_opt) $(def) -zq -bt=dos $(incdir)

$(alib): $(obj)
	%write objects.lbc $(obj)
	wlib -b -n $@ @objects

.c: src

.c.obj: .autodepend
	%write cflags.occ $(CFLAGS)
	$(CC) -fo=$@ @cflags.occ $[*

!ifdef __UNIX__
clean: .symbolic
	rm -f $(obj) $(alib)
!else
clean: .symbolic
	del src\*.obj
	del $(alib)
!endif
