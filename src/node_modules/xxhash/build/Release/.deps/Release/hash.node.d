cmd_Release/hash.node := ln -f "Release/obj.target/hash.node" "Release/hash.node" 2>/dev/null || (rm -rf "Release/hash.node" && cp -af "Release/obj.target/hash.node" "Release/hash.node")
