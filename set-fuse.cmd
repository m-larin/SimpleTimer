set AVRDUDE=C:\avr\avrdude-6.3-mingw32\avrdude.exe

rem set lfuse тактование от кварца 3-8 МГц
%AVRDUDE% -c usbasp -p m8 -B12 -U lfuse:w:0xEF:m

rem set lock bit
rem %AVRDUDE% -c usbasp -p m8 -B12 -U lock:w:0xFF:m

pause