IMATH_DIR_321=/home/peteru/Imath-3.1.9_2
IMATH_INC_321=$(IMATH_DIR_321)/include/Imath
IMATH_LIB_321=$(IMATH_DIR_321)/lib
OPENEXR_DIR_321=/home/peteru/openexr-3.2.1
OPENEXR_INC_321=$(OPENEXR_DIR_321)/include/OpenEXR
OPENEXR_LIB_321=$(OPENEXR_DIR_321)/lib

IMATH_DIR_331=/home/peteru/Imath-3.1.12
IMATH_INC_331=$(IMATH_DIR_331)/include/Imath
IMATH_LIB_331=$(IMATH_DIR_331)/lib
OPENEXR_DIR_331=/home/peteru/openexr-3.3.1
OPENEXR_INC_331=$(OPENEXR_DIR_331)/include/OpenEXR
OPENEXR_LIB_331=$(OPENEXR_DIR_331)/lib

# CLAMG_DIR=/home/peteru/clang-12.0.1
# CXX=$(CLAMG_DIR)/bin/clang++
# CXXFLAGS=-stdlib=libc++
LDFLAGS_321=-Wl,-rpath,$(OPENEXR_LIB_321):$(IMATH_LIB_321):$(CLAMG_DIR)/lib -L$(IMATH_LIB_321) -L$(OPENEXR_LIB_321)
LDFLAGS_331=-Wl,-rpath,$(OPENEXR_LIB_331):$(IMATH_LIB_331):$(CLAMG_DIR)/lib -L$(IMATH_LIB_331) -L$(OPENEXR_LIB_331)

all: exrmetrics_321 exrmetrics_331 test

exrmetrics_321.o: exrmetrics.cpp exrmetrics.h
	$(CXX) $(CXXFLAGS)\
		-I$(IMATH_INC_321) -I$(OPENEXR_INC_321) \
		-c -o exrmetrics_321.o exrmetrics.cpp

main_321.o: main.cpp
	$(CXX) $(CXXFLAGS)\
		-I$(IMATH_INC_321) -I$(OPENEXR_INC_321) \
		-c -o main_321.o main.cpp

exrmetrics_321: exrmetrics_321.o  main_321.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS_321) \
		-lImath -lOpenEXR -lIex \
		-o exrmetrics_321 main_321.o exrmetrics_321.o

exrmetrics_331.o: exrmetrics.cpp exrmetrics.h
	$(CXX) $(CXXFLAGS)\
		-I$(IMATH_INC_331) -I$(OPENEXR_INC_331) \
		-c -o exrmetrics_331.o exrmetrics.cpp

main_331.o: main.cpp
	$(CXX) $(CXXFLAGS)\
		-I$(IMATH_INC_331) -I$(OPENEXR_INC_331) \
		-c -o main_331.o main.cpp

exrmetrics_331: exrmetrics_331.o  main_331.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS_331) \
		-lImath -lOpenEXR -lIex \
		-o exrmetrics_331 main_331.o exrmetrics_331.o

clean:
	@rm -f exrmetrics_321 exrmetrics_321.o  main_321.o
	@rm -f exrmetrics_331 exrmetrics_331.o  main_331.o

test: exrmetrics_321 exrmetrics_331
	@echo "OpenEXR 3.2.1"
	@./exrmetrics_321 -z none  -16 all test_image.exr  none-half.exr   | grep 'write'
	@./exrmetrics_321 -z none          test_image.exr  none-float.exr  | grep 'write'
	@./exrmetrics_321 -z rle   -16 all test_image.exr  rle-half.exr    | grep 'write'
	@./exrmetrics_321 -z rle           test_image.exr  rle-float.exr   | grep 'write'
	@./exrmetrics_321 -z zips  -16 all test_image.exr  zips-half.exr   | grep 'write'
	@./exrmetrics_321 -z zips          test_image.exr  zips-float.exr  | grep 'write'
	@./exrmetrics_321 -z zip   -16 all test_image.exr  zip-half.exr    | grep 'write'
	@./exrmetrics_321 -z zip           test_image.exr  zip-float.exr   | grep 'write'
	@./exrmetrics_321 -z piz   -16 all test_image.exr  piz-half.exr    | grep 'write'
	@./exrmetrics_321 -z piz           test_image.exr  piz-float.exr   | grep 'write'
	@./exrmetrics_321 -z pxr24 -16 all test_image.exr  pxr24-half.exr  | grep 'write'
	@./exrmetrics_321 -z pxr24         test_image.exr  pxr24-float.exr | grep 'write'
	@./exrmetrics_321 -z b44   -16 all test_image.exr  b44-half.exr    | grep 'write'
	@./exrmetrics_321 -z b44           test_image.exr  b44-float.exr   | grep 'write'
	@./exrmetrics_321 -z b44a  -16 all test_image.exr  b44a-half.exr   | grep 'write'
	@./exrmetrics_321 -z b44a          test_image.exr  b44a-float.exr  | grep 'write'
	@./exrmetrics_321 -z dwaa  -16 all test_image.exr  dwaa-half.exr   | grep 'write'
	@./exrmetrics_321 -z dwaa          test_image.exr  dwaa-float.exr  | grep 'write'
	@./exrmetrics_321 -z dwab  -16 all test_image.exr  dwab-half.exr   | grep 'write'
	@./exrmetrics_321 -z dwab          test_image.exr  dwab-float.exr  | grep 'write'
	@echo "OpenEXR 3.3.1"
	@./exrmetrics_331 -z none  -16 all test_image.exr  none-half.exr   | grep 'write'
	@./exrmetrics_331 -z none          test_image.exr  none-float.exr  | grep 'write'
	@./exrmetrics_331 -z rle   -16 all test_image.exr  rle-half.exr    | grep 'write'
	@./exrmetrics_331 -z rle           test_image.exr  rle-float.exr   | grep 'write'
	@./exrmetrics_331 -z zips  -16 all test_image.exr  zips-half.exr   | grep 'write'
	@./exrmetrics_331 -z zips          test_image.exr  zips-float.exr  | grep 'write'
	@./exrmetrics_331 -z zip   -16 all test_image.exr  zip-half.exr    | grep 'write'
	@./exrmetrics_331 -z zip           test_image.exr  zip-float.exr   | grep 'write'
	@./exrmetrics_331 -z piz   -16 all test_image.exr  piz-half.exr    | grep 'write'
	@./exrmetrics_331 -z piz           test_image.exr  piz-float.exr   | grep 'write'
	@./exrmetrics_331 -z pxr24 -16 all test_image.exr  pxr24-half.exr  | grep 'write'
	@./exrmetrics_331 -z pxr24         test_image.exr  pxr24-float.exr | grep 'write'
	@./exrmetrics_331 -z b44   -16 all test_image.exr  b44-half.exr    | grep 'write'
	@./exrmetrics_331 -z b44           test_image.exr  b44-float.exr   | grep 'write'
	@./exrmetrics_331 -z b44a  -16 all test_image.exr  b44a-half.exr   | grep 'write'
	@./exrmetrics_331 -z b44a          test_image.exr  b44a-float.exr  | grep 'write'
	@./exrmetrics_331 -z dwaa  -16 all test_image.exr  dwaa-half.exr   | grep 'write'
	@./exrmetrics_331 -z dwaa          test_image.exr  dwaa-float.exr  | grep 'write'
	@./exrmetrics_331 -z dwab  -16 all test_image.exr  dwab-half.exr   | grep 'write'
	@./exrmetrics_331 -z dwab          test_image.exr  dwab-float.exr  | grep 'write'
	@echo "OpenEXR 3.2.1"
	@./exrmetrics_321 none-half.exr   /dev/null | grep 'read'
	@./exrmetrics_321 none-float.exr  /dev/null | grep 'read'
	@./exrmetrics_321 rle-half.exr    /dev/null | grep 'read'
	@./exrmetrics_321 rle-float.exr   /dev/null | grep 'read'
	@./exrmetrics_321 zips-half.exr   /dev/null | grep 'read'
	@./exrmetrics_321 zips-float.exr  /dev/null | grep 'read'
	@./exrmetrics_321 zip-half.exr    /dev/null | grep 'read'
	@./exrmetrics_321 zip-float.exr   /dev/null | grep 'read'
	@./exrmetrics_321 piz-half.exr    /dev/null | grep 'read'
	@./exrmetrics_321 piz-float.exr   /dev/null | grep 'read'
	@./exrmetrics_321 pxr24-half.exr  /dev/null | grep 'read'
	@./exrmetrics_321 pxr24-float.exr /dev/null | grep 'read'
	@./exrmetrics_321 b44-half.exr    /dev/null | grep 'read'
	@./exrmetrics_321 b44-float.exr   /dev/null | grep 'read'
	@./exrmetrics_321 b44a-half.exr   /dev/null | grep 'read'
	@./exrmetrics_321 b44a-float.exr  /dev/null | grep 'read'
	@./exrmetrics_321 dwaa-half.exr   /dev/null | grep 'read'
	@./exrmetrics_321 dwaa-float.exr  /dev/null | grep 'read'
	@./exrmetrics_321 dwab-half.exr   /dev/null | grep 'read'
	@./exrmetrics_321 dwab-float.exr  /dev/null | grep 'read'
	@echo "OpenEXR 3.3.1"
	@./exrmetrics_331 none-half.exr   /dev/null | grep 'read'
	@./exrmetrics_331 none-float.exr  /dev/null | grep 'read'
	@./exrmetrics_331 rle-half.exr    /dev/null | grep 'read'
	@./exrmetrics_331 rle-float.exr   /dev/null | grep 'read'
	@./exrmetrics_331 zips-half.exr   /dev/null | grep 'read'
	@./exrmetrics_331 zips-float.exr  /dev/null | grep 'read'
	@./exrmetrics_331 zip-half.exr    /dev/null | grep 'read'
	@./exrmetrics_331 zip-float.exr   /dev/null | grep 'read'
	@./exrmetrics_331 piz-half.exr    /dev/null | grep 'read'
	@./exrmetrics_331 piz-float.exr   /dev/null | grep 'read'
	@./exrmetrics_331 pxr24-half.exr  /dev/null | grep 'read'
	@./exrmetrics_331 pxr24-float.exr /dev/null | grep 'read'
	@./exrmetrics_331 b44-half.exr    /dev/null | grep 'read'
	@./exrmetrics_331 b44-float.exr   /dev/null | grep 'read'
	@./exrmetrics_331 b44a-half.exr   /dev/null | grep 'read'
	@./exrmetrics_331 b44a-float.exr  /dev/null | grep 'read'
	@./exrmetrics_331 dwaa-half.exr   /dev/null | grep 'read'
	@./exrmetrics_331 dwaa-float.exr  /dev/null | grep 'read'
	@./exrmetrics_331 dwab-half.exr   /dev/null | grep 'read'
	@./exrmetrics_331 dwab-float.exr  /dev/null | grep 'read'

