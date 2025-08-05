#!/bin/bash

# JavaScriptのconstとletをvarに置き換える（client.println内のみ）
sed -i '' 's/client\.println(F("let /client.println(F("var /g' src/network/webserver.cpp
sed -i '' 's/client\.println(F("  let /client.println(F("  var /g' src/network/webserver.cpp
sed -i '' 's/client\.println(F("    let /client.println(F("    var /g' src/network/webserver.cpp
sed -i '' 's/client\.println(F("      let /client.println(F("      var /g' src/network/webserver.cpp
sed -i '' 's/client\.println("  let /client.println("  var /g' src/network/webserver.cpp

# JavaScriptのconstをvarに置き換える（client.println内のみ）
sed -i '' 's/client\.println(F("  const /client.println(F("  var /g' src/network/webserver.cpp
sed -i '' 's/client\.println(F("    const /client.println(F("    var /g' src/network/webserver.cpp
sed -i '' 's/client\.println(F("      const /client.println(F("      var /g' src/network/webserver.cpp
sed -i '' 's/client\.println("  const /client.println("  var /g' src/network/webserver.cpp

# forループ内のletを修正
sed -i '' 's/for (let /for (var /g' src/network/webserver.cpp

echo "const/let -> var replacement completed"