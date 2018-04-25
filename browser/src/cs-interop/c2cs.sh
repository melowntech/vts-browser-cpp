#!/bin/bash

LIB=$1

# read input
cat /dev/stdin | \

# remove /* */ comments -- wont work with comments in strings -- dont care
awk -vRS='*/' '{gsub(/\/\*.*/,"")}1' - | \

# remove // comments
awk '{gsub(/\ ?\/\/.*/,"")} 1' - | \

# remove #macros
grep -v '^#' | \

# remove extern "c"
grep -v '^extern "C" {' | \
grep -v '^} // extern C' | \

# callback types
sed 's/typedef \(.*\)(\*\([[:alnum:]]\+\))(\(.*\))/\[UnmanagedFunctionPointer(CallingConvention.Cdecl)]\npublic delegate \1 \2(\3)/' - | \

# remove typedefs
grep -v '^typedef .*;$' | \

# remove { } blocks
awk -vRS='}' '{gsub(/{.*/,"")} 1' - | \

# remove leftover typedefs & enums
awk -vRS=';' -vORS=';' '{gsub(/typedef.*/,"")} 1' - | \
awk -vRS=';' -vORS=';' '{gsub(/enum.*/,"")} 1' - | \

# squash all parameters into single line
awk -vRS=',\n' -vORS=',' '1' - | \
tr -s " " | \

# remove leftover ; and ,
grep -v '^[;\,]*$' | \

# remove empty lines
grep -v '^ *$' | \

# replace handles
awk '{gsub(/vtsH[^ ]*/,"IntPtr")} 1' - | \

# replace ints
awk '{gsub(/uint32/,"int")} 1' - | \
awk '{gsub(/sint32/,"uint")} 1' - | \

# DllImport
awk -vLIB="$LIB" '{sub(/VTS_API/,"[DllImport(\""LIB"\", CallingConvention = CallingConvention.Cdecl)]\nVTS_API")} 1' - | \

# return string
awk '{sub(/VTS_API const char \*/,"VTS_API IntPtr ")} 1' - | \
awk '{sub(/delegate const char \*/,"delegate IntPtr ")} 1' - | \

# return void*
awk '{sub(/VTS_API void \*/,"VTS_API IntPtr ")} 1' - | \

# return struct*
sed 's/VTS_API const \([[:alnum:]]\+\) \*/VTS_API IntPtr /g' - | \
sed 's/VTS_API \([[:alnum:]]\+\) \*/VTS_API IntPtr /g' - | \

# parameter string
awk '{gsub(/const char \*/,"[MarshalAs(UnmanagedType.LPStr)] string ")} 1' - | \

# parameter array
sed 's/\(const \)\?\([[:alnum:]]\+\) \([[:alnum:]]\+\)\[\([[:digit:]]\+\)\]/INOUTMARK\1 \2\[\] \3/g' - | \
sed 's/INOUTMARKconst /\[In\]/g' - | \
sed 's/INOUTMARK/\[Out\]/g' - | \

# parameter void*
awk '{gsub(/void \*/,"IntPtr ")} 1' - | \

# out parameter
sed 's/\([[:alnum:]]\+\) \*\([[:alnum:]]\+\)/out \1 \2/g' - | \

# static extern
awk '{sub(/VTS_API/,"public static extern")} 1' - | \

# add empty lines
awk '{sub(/;/,";\n")} 1' - | \

# add preamble and postamble
cat ./begin.cs - ./end.cs | \

# print output
cat -


