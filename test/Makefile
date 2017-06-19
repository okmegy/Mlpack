cc=g++
PROJECT_ROOT=/home/jasper/mlpack-2.2.3
CFLAGS:= -std=c++11 -Wall -fPIC -O2
C_INC:= /home/jasper/Mlpack/build


Test.out:Test.o
	${cc} ${CFLAGS} -o Test.out Test.o -larmadillo -lmlpack -L{$C_INC/lib} -lboost_serialization -L/usr/lib/x86_64-linux-gnu

Test.o:Test.cpp
	${cc} -I{$C_INC/include} ${CFLAGS}  -c Test.cpp

run:Test.out
	./Test.out
clean:
	rm -rf *.o *.out *.so
