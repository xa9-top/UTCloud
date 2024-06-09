<?php

define('TOKEN', '123456'); // 这里是定义 token 的位置，用来验证你的身份
define('FILE_PATH', './UNDERTALE.zip'); // 这里是定义 file_path 的位置，用来保存和读取你的存档文件的地方，注意要确保php对这个路径有权限

// 设置错误处理
set_error_handler(function($errno, $errstr, $errfile, $errline) {
    http_response_code(500);
    echo "Server error: $errstr";
    exit();
});

// 获取请求方法和 token 参数
$requestMethod = $_SERVER['REQUEST_METHOD'];
$token = $_REQUEST['token'] ?? '';

// 验证 token
if ($token !== TOKEN) {
    http_response_code(401);
    echo "Unable to authenticate";
    exit();
}

if ($requestMethod === 'GET') {
    // 处理 GET 请求，返回文件 FILE_PATH
    if (file_exists(FILE_PATH)) {
        header('Content-Type: application/zip');
        header('Content-Disposition: attachment; filename="ut.zip"');
        header('Content-Length: ' . filesize(FILE_PATH));
        readfile(FILE_PATH);
        exit();
    } else {
        http_response_code(410);
        echo "Server error: No such file: '".FILE_PATH."'";
        exit();
    }
} elseif ($requestMethod === 'POST') {
    // 处理 POST 请求，保存上传的文件到 FILE_PATH
    if (isset($_FILES['file'])) {
        if (move_uploaded_file($_FILES['file']['tmp_name'], FILE_PATH)) {
            echo "1";
        } else {
            http_response_code(500);
            echo "Server error: failed to file upload";
        }
        exit();
    } else {
        http_response_code(408);
        echo "No file received";
        exit();
    }
} else {
    http_response_code(400);
    echo "Server error: Invalid request method";
    exit();
}
?>
