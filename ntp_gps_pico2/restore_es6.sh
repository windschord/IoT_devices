#!/bin/bash

# Chrome専用なので、ES6+の機能を復活させる
sed -i '' 's/client\.println(F("var /client.println(F("const /g' src/network/webserver.cpp
sed -i '' 's/client\.println(F("  var /client.println(F("  let /g' src/network/webserver.cpp

echo "ES6+ const/let restoration completed for Chrome"