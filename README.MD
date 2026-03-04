


<html lang="ko">
<head>
<meta charset="UTF-8">
<title>Folder 다운로드 페이지</title>

<style>
body {
    font-family: Arial, sans-serif;
    padding: 20px;
    margin: 0;
}

/* 이미지 두 개를 양 옆으로 배치하는 컨테이너 */
.image-container {
    display: flex;
    justify-content: flex-start;
    gap: 20px;
    margin-bottom: 20px;
}

.image-wrapper {
    text-align: center;
}

img {
    display: block;
    max-width: 300px;
    height: auto;
    border: 1px solid #ccc;
}

.links {
    margin-top: 20px;
}

a {
    display: block;
    margin: 10px 0;
    font-size: 18px;
    color: #0066cc;
    text-decoration: none;
}

a:hover {
    text-decoration: underline;
}
</style>
</head>

<body>

<div class="image-container">
    <div class="image-wrapper">
        <p>첫 번째 그림 (왼쪽)</p>
        <img src="screen.png" alt="스크린샷 1">
    </div>
    <div class="image-wrapper">
        <p>두 번째 그림 (오른쪽)</p>
        <img src="screen2.png" alt="스크린샷 2">
    </div>
</div>

<hr>

<div class="links">
    <h1>Folder 다운로드</h1>
    <a href="folder1.0.zip">📂 1. folder1.0 압축 풀고 deb 설치</a>
    <a href="folder.zip">📂 2. 수정 파일 업그레이드 (folder.zip)</a>
</div>

<p><strong>설치 명령어:</strong></p>
<code>sudo mv folder /usr/local/bin</code>

</body>
</html>
