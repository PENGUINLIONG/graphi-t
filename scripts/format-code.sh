#! /usr/bin/bash
find apps -iname *.h -o -iname *.hpp -o -iname *.cpp | xargs clang-format -i
find src -iname *.h -o -iname *.hpp -o -iname *.cpp | xargs clang-format -i
find include -iname *.h -o -iname *.hpp -o -iname *.cpp | xargs clang-format -i
