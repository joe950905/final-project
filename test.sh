#!/bin/bash

# File Server 測試腳本
# 這個腳本會自動測試所有API功能

echo "=================================="
echo "  File Server API 測試腳本"
echo "=================================="
echo ""

API_URL="http://localhost:7878"

# 檢查服務器是否運行
echo "1. 檢查服務器狀態..."
if ! curl -s --connect-timeout 2 $API_URL > /dev/null; then
    echo "❌ 錯誤：服務器未運行！"
    echo "請先啟動服務器：./fileserver"
    exit 1
fi
echo "✓ 服務器正在運行"
echo ""

# 登錄
echo "2. 測試登錄..."
LOGIN_RESPONSE=$(curl -s -X POST $API_URL/login \
    -d 'username=admin&password=admin123')

if echo "$LOGIN_RESPONSE" | grep -q "Token:"; then
    TOKEN=$(echo "$LOGIN_RESPONSE" | grep "Token:" | awk '{print $2}' | tr -d '\n\r')
    echo "✓ 登錄成功"
    echo "Token: $TOKEN"
else
    echo "❌ 登錄失敗"
    echo "$LOGIN_RESPONSE"
    exit 1
fi
echo ""

# 創建測試文件
echo "3. 創建測試文件..."
echo "Hello from test script!" > /tmp/test_file.txt
echo "This is a test file created by the automated test script." >> /tmp/test_file.txt
echo "It contains multiple lines of text." >> /tmp/test_file.txt
echo "✓ 測試文件已創建"
echo ""

# 上傳文件
echo "4. 測試文件上傳..."
FILE_CONTENT=$(cat /tmp/test_file.txt)
UPLOAD_RESPONSE=$(curl -s -X POST $API_URL/files \
    -H "Authorization: Bearer $TOKEN" \
    -d "filename=test_upload.txt&data=$FILE_CONTENT")

if echo "$UPLOAD_RESPONSE" | grep -q "uploaded successfully"; then
    echo "✓ 文件上傳成功"
    echo "$UPLOAD_RESPONSE"
else
    echo "❌ 文件上傳失敗"
    echo "$UPLOAD_RESPONSE"
fi
echo ""

# 列出文件
echo "5. 測試列出文件..."
LIST_RESPONSE=$(curl -s $API_URL/files \
    -H "Authorization: Bearer $TOKEN")

if echo "$LIST_RESPONSE" | grep -q "Available files"; then
    echo "✓ 列出文件成功"
    echo "$LIST_RESPONSE"
else
    echo "❌ 列出文件失敗"
    echo "$LIST_RESPONSE"
fi
echo ""

# 下載文件
echo "6. 測試文件下載..."
curl -s $API_URL/files/test_upload.txt \
    -H "Authorization: Bearer $TOKEN" \
    -o /tmp/downloaded_file.txt

if [ -f /tmp/downloaded_file.txt ]; then
    echo "✓ 文件下載成功"
    echo "下載的文件內容："
    cat /tmp/downloaded_file.txt
else
    echo "❌ 文件下載失敗"
fi
echo ""

# 列出用戶（管理員功能）
echo "7. 測試列出用戶（管理員功能）..."
USERS_RESPONSE=$(curl -s $API_URL/users \
    -H "Authorization: Bearer $TOKEN")

if echo "$USERS_RESPONSE" | grep -q "Users:"; then
    echo "✓ 列出用戶成功"
    echo "$USERS_RESPONSE"
else
    echo "❌ 列出用戶失敗"
    echo "$USERS_RESPONSE"
fi
echo ""

# 創建新用戶（管理員功能）
echo "8. 測試創建新用戶（管理員功能）..."
CREATE_USER_RESPONSE=$(curl -s -X POST $API_URL/users \
    -H "Authorization: Bearer $TOKEN" \
    -d "username=testuser&password=test123")

if echo "$CREATE_USER_RESPONSE" | grep -q "created successfully"; then
    echo "✓ 創建用戶成功"
    echo "$CREATE_USER_RESPONSE"
else
    echo "⚠ 創建用戶失敗（可能已存在）"
    echo "$CREATE_USER_RESPONSE"
fi
echo ""

# 使用新用戶登錄
echo "9. 測試新用戶登錄..."
NEW_LOGIN_RESPONSE=$(curl -s -X POST $API_URL/login \
    -d 'username=testuser&password=test123')

if echo "$NEW_LOGIN_RESPONSE" | grep -q "Token:"; then
    NEW_TOKEN=$(echo "$NEW_LOGIN_RESPONSE" | grep "Token:" | awk '{print $2}' | tr -d '\n\r')
    echo "✓ 新用戶登錄成功"
    echo "New Token: $NEW_TOKEN"
else
    echo "❌ 新用戶登錄失敗"
    echo "$NEW_LOGIN_RESPONSE"
fi
echo ""

# 測試新用戶權限（應該無法刪除文件）
echo "10. 測試普通用戶權限（嘗試刪除文件）..."
DELETE_RESPONSE=$(curl -s -X DELETE $API_URL/files/test_upload.txt \
    -H "Authorization: Bearer $NEW_TOKEN")

if echo "$DELETE_RESPONSE" | grep -q "Forbidden"; then
    echo "✓ 權限控制正常（普通用戶無法刪除文件）"
    echo "$DELETE_RESPONSE"
else
    echo "⚠ 權限控制異常"
    echo "$DELETE_RESPONSE"
fi
echo ""

# 使用管理員刪除文件
echo "11. 測試管理員刪除文件..."
DELETE_ADMIN_RESPONSE=$(curl -s -X DELETE $API_URL/files/test_upload.txt \
    -H "Authorization: Bearer $TOKEN")

if echo "$DELETE_ADMIN_RESPONSE" | grep -q "deleted successfully"; then
    echo "✓ 管理員刪除文件成功"
    echo "$DELETE_ADMIN_RESPONSE"
else
    echo "❌ 刪除文件失敗"
    echo "$DELETE_ADMIN_RESPONSE"
fi
echo ""

# 刪除測試用戶
echo "12. 測試刪除用戶（管理員功能）..."
DELETE_USER_RESPONSE=$(curl -s -X DELETE $API_URL/users \
    -H "Authorization: Bearer $TOKEN" \
    -d "username=testuser")

if echo "$DELETE_USER_RESPONSE" | grep -q "deleted successfully"; then
    echo "✓ 刪除用戶成功"
    echo "$DELETE_USER_RESPONSE"
else
    echo "❌ 刪除用戶失敗"
    echo "$DELETE_USER_RESPONSE"
fi
echo ""

# 測試總結
echo "=================================="
echo "         測試完成！"
echo "=================================="
echo ""
echo "測試項目："
echo "  ✓ 服務器連接"
echo "  ✓ 用戶登錄"
echo "  ✓ 文件上傳"
echo "  ✓ 文件列表"
echo "  ✓ 文件下載"
echo "  ✓ 用戶列表"
echo "  ✓ 創建用戶"
echo "  ✓ 權限控制"
echo "  ✓ 刪除文件"
echo "  ✓ 刪除用戶"
echo ""
echo "所有主要功能已測試完成！"
echo ""

# 清理臨時文件
rm -f /tmp/test_file.txt /tmp/downloaded_file.txt
