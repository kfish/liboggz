
CC = @cl
LD = @link
CFLAGS = /nologo /c /FD /W1 /YX /I"." /I"..\src\liboggz" /I"..\include" /I"..\..\ogg\include" /D "WIN32"
DLL_LFLAGS = /nologo ogg.lib /DLL /LIBPATH:"..\..\ogg\win32\Dynamic_Release"
EXE_LFLAGS = /nologo

!ifdef DEBUG
OBJ = Debug
CFLAGS = $(CFLAGS) /MDd /Od /Yd
!else
OBJ = Release
CFLAGS = $(CFLAGS) /MD /O2
!endif

DLL = $(OBJ)\liboggz.dll

EXECUTABLES = $(OBJ)\oggzdump.exe $(OBJ)\oggzed.exe $(OBJ)\oggzinfo.exe $(OBJ)\oggzmerge.exe

DLL_OBJECTS = $(OBJ)\oggz.obj $(OBJ)\oggz_io.obj $(OBJ)\oggz_read.obj $(OBJ)\oggz_write.obj \
	$(OBJ)\oggz_auto.obj $(OBJ)\oggz_table.obj $(OBJ)\oggz_vector.obj

EXE_OBJECTS = $(OBJ)\oggzdump.obj $(OBJ)\oggzed.obj $(OBJ)\oggzinfo.obj $(OBJ)\oggzmerge.obj

All: $(OBJ) $(DLL) $(EXECUTABLES)

$(OBJ):
	@mkdir $(OBJ)

$(DLL): $(DLL_OBJECTS)
	$(LD) $(DLL_OBJECTS) $(DLL_LFLAGS) /DEF:liboggz.def /IMPLIB:$*.lib /OUT:$@

$(OBJ)\oggzdump.exe : $(OBJ)\oggzdump.obj $(OBJ)\attgetopt.obj
	@echo Linking $@...
	$(LD) $*.obj $(OBJ)\attgetopt.obj $(EXE_LFLAGS) $(OBJ)\liboggz.lib /OUT:$@

$(OBJ)\oggzed.exe : $(OBJ)\oggzed.obj $(OBJ)\attgetopt.obj
	@echo Linking $@...
	$(LD) $*.obj $(OBJ)\attgetopt.obj $(EXE_LFLAGS) $(OBJ)\liboggz.lib /OUT:$@

$(OBJ)\oggzinfo.exe : $(OBJ)\oggzinfo.obj $(OBJ)\attgetopt.obj
	@echo Linking $@...
	$(LD) $*.obj $(OBJ)\attgetopt.obj $(EXE_LFLAGS) $(OBJ)\liboggz.lib /OUT:$@

$(OBJ)\oggzmerge.exe : $(OBJ)\oggzmerge.obj $(OBJ)\attgetopt.obj
	@echo Linking $@...
	$(LD) $*.obj $(OBJ)\attgetopt.obj $(EXE_LFLAGS) $(OBJ)\liboggz.lib /OUT:$@

{..\src\liboggz}.c{$(OBJ)}.obj :
	$(CC) $(CFLAGS) /Fo$*.obj $<

{..\src\tools}.c{$(OBJ)}.obj :
	$(CC) $(CFLAGS) /Fo$*.obj $<

.c{$(OBJ)}.obj :
	$(CC) $(CFLAGS) /Fo$*.obj $<

clean:
	@del /sexyq $(OBJ)
	@del /q vc60.*
