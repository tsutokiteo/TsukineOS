#!/bin/bash
echo "开始批量编译 TsukineOS ARM 工具..."

# 检查必要的静态库
pkg install ncurses-static 2>/dev/null || echo "ncurses-static 安装跳过"

# 1. 网络工具
clang -o tsukine_ping_arm tsukine_ping.c
echo "✅ tsukine_ping_arm"

clang -o tsukine_net_arm tsukine_net.c
echo "✅ tsukine_net_arm"

clang -o tsukine_net_ui_arm tsukine_net_ui.c
echo "✅ tsukine_net_ui_arm"

# 2. 系统工具
clang -o tsukine_iwr_arm tsukine_iwr.c
echo "✅ tsukine_iwr_arm"

clang -o tsukine_reboot_arm tsukine_reboot.c
echo "✅ tsukine_reboot_arm"

clang -o tsukine_shutdown_arm tsukine_shutdown.c
echo "✅ tsukine_shutdown_arm"

# 3. 任务管理器（需要 ncurses）
clang -o tsukine_top_arm tsukine_top.c -lncurses -ltinfo 2>/dev/null && \
    echo "✅ tsukine_top_arm" || echo "❌ tsukine_top_arm (ncurses missing)"

echo ""
echo "编译完成！可执行文件："
ls -lh tsukine_*_arm 2>/dev/null
