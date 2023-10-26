# 0. 使用MinIO搭建私有S3
## 下载并启动minio服务：
```shell
$ wget https://dl.minio.io/server/minio/release/linux-amd64/minio
$ chmod +x minio
$ mkdir -p /data/s3_store
$ ./minio server /data/s3_store
```

## 通过浏览器访问http://127.0.0.1:9000/进行管理：
创建access key，包括<ak>和<sk>。

创建bucket，名称为ts-bucket。

设置Server Location，名称为cn-hz-xxlang。


# 1. s3-upload-aws4.sh
## 运行脚本上传一个文件到S3：
```shell
export AWS_ACCESS_KEY_ID='<ak>'
export AWS_SECRET_ACCESS_KEY='<sk>'
[AWS_UPLOAD=0|1|2] ./s3-upload-aws4.sh '<local-file>' '<key-prefix>' '<bucket-name>' '<region-code>'
```
`AWS_UPLOAD`:

* `0` 不上传
* `1` 上传但不输出日志
* `2` 上传并且输出日志

脚本内部根据key-prefix和当前时间按照以下格式拼装key-name：
```shell
<key-name>=<key-prefix>/<年:YYYY>/<月:mm>/<日:dd>/<时:HH>/<分:MM>/<秒:SS>.ts
```

### 私有S3测试：
```shell
AWS_UPLOAD=2 ./s3-upload-aws4.sh 'test.ts' 'ipc1' 'http://127.0.0.1:9000/ts-bucket/'
```

### AWS S3测试：
```shell
AWS_UPLOAD=1 ./s3-upload-aws4.sh 'test.ts' 'ipc1' 'ipc-video-bucket-626676147343-us-east-1' 'us-east-1'
```
目前，Amazon S3 在所有 AWS 区域中同时支持虚拟托管类型和路径类型 URL 访问。但是，路径类型 URL 将来会停用。
AWS S3 路径类型 URL 格式：
```shell
https://s3.<region-code>.amazonaws.com/<bucket-name>/<key-name>
```
在虚拟托管类型 URI 中，存储桶名称是 URL 中域名的一部分。AWS S3 虚拟托管类型 URL 格式：
```shell
https://<bucket-name>.s3.<region-code>.amazonaws.com/<key-name>
```
Host Header值设置为URL中的域名部分，即
```shell
host:<bucket-name>.s3.<region-code>
```
上面的host行是经过Canonical处理的，处理方式为：
```shell
LowerCase(<HeaderName>) + ":" + Trim(<HeaderValue>) + "\n"
```

## 重点代码解释：
### Hash the file to be uploaded
```shell
payloadHash=$(my_openssl dgst -sha256 -hex < "${fileLocal}" 2>/dev/null | my_sed 's/^.* //')
```
作用是对`fileLocal`文件内容使用SHA256算法做digest，再将digest结果以hex格式输出。
openssl输出结果为：
```shell
$ openssl dgst -sha256 -hex < test.xml 2>/dev/null
(stdin)= b200e431329390d46bb29a048ef407f1d5dae058ea43713ca0f20fde6a80a316
```
需要将前面多余的‘(stdin)= ’去掉作为payloadHash，处理后的payloadHash长度为64字节。

### Create canonical request
```shell
canonicalRequest="\
${httpReq}
/${pathRemote}
                                   // 这里空行，表示CanonicalQueryString部分为空，而CanonicalQueryString之后需要一个换行。
content-type:${contentType}
host:${hostport}
x-amz-content-sha256:${payloadHash}
x-amz-date:${dateValueL}
x-amz-storage-class:${storageClass}
                                   // 这里空行，是因为每个CanonicalHeader之后都需要一个换行，所有CanonicalHeaders之后还需要一个换行。
${headerList}
${payloadHash}"                    // 注意HashedPayload后面要求没有换行。
```

#### Sign the string
```shell
openssl dgst -sha256 -hex -mac HMAC -macopt "key:${kSecret}"
openssl dgst -sha256 -hex -mac HMAC -macopt "hexkey:${kDate}"
```

openssl dgst默认输出格式是hex。

第一层dgst，由于采用字符串格式的输入数据作为MAC算法参数，所以macopt设置为key。

第二层到最后一层dgst，由于都是采用上一层输出的hex格式结果作为MAC算法参数，所以macopt设置都为hexkey。这个流程似乎与AWS文档不一致，以下证明二者效果一样。

假设`<sk>`为'example_sk'

（1）AWS文档方式，将字符串格式的<sk>设置到macopt的key，得到的dgst为：
```shell
printf 'aws4_request' | openssl dgst -sha256 -hex -mac HMAC -macopt "key:AWS4example_sk"
(stdin)= 3fb214795259cf8f52eaaced84baa2d7eeba13fd03a0251cbb016b58166ed915
```

（2）本脚本方式，先将<sk>转换为hex
```shell
$ printf 'AWS4example_sk' | hexdump -v -e '/1 "%02x"'
415753346578616d706c655f736b
```
再将hex格式的<sk>设置到macopt的hexkey，得到的dgst为：
```shell
$ printf 'aws4_request' | openssl dgst -sha256 -hex -mac HMAC -macopt "hexkey:415753346578616d706c655f736b"
(stdin)= 3fb214795259cf8f52eaaced84baa2d7eeba13fd03a0251cbb016b58166ed915
```

### Upload the file
```shell
curl --location --proto-redir =https --request "${httpReq}" --upload-file "${fileLocal}" --header "${fullUrl}"
```
参数解释：
* --location：支持重定向
* --proto-redir =https：只允许重定向到https

(1) 查看带重定向的网页：
```shell
$ curl haicoder.net
```
由于访问的 url 是带重定向的，因此这里只显示了重定向信息，并没有显示网页内容。
```html
<html>
<head><title>301 Moved Permanently</title></head>
<body bgcolor="white">
<center><h1>301 Moved Permanently</h1></center>
<hr><center>nginx</center>
</body>
</html>
```
(2) 使用 --location 参数，显示重定向后的网页内容：
```shell
$ curl --location haicoder.net
```
(3) 使用 --proto-redir 参数，限制只能重定向到http，则无法重定向到https页面：
```shell
$ curl --location --proto-redir =http haicoder.net
curl: (1) Protocol "https" not supported or disabled in libcurl
```

TODO: upload data instead of file
TODO: 超时参数
TODO: 重传参数

# 2. s3-hls-server
通过浏览器点播：
```
http://localhost:8081/replay.m3u8?camera_id=ipc1&start=2023-10-20%2008:01:01&end=2023-10-20%2008:02:02
```
