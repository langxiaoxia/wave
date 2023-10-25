#!/bin/sh

# Upload a file to AWS S3 using Signature Version 4.
#
# docs:
#   https://docs.aws.amazon.com/general/latest/gr/sigv4-create-canonical-request.html
#   https://docs.aws.amazon.com/AmazonS3/latest/API/sigv4-query-string-auth.html
#
# requires:
#   curl, openssl 1.x or newer, GNU sed, LF EOLs in this file
#
# usage:
#   export AWS_ACCESS_KEY_ID='<ak>'
#   export AWS_SECRET_ACCESS_KEY='<sk>'
#   AWS_UPLOAD=1 ./s3-upload-aws4.sh 'test.ts' 'ipc1' 'http://127.0.0.1:9000/ts-bucket/'


set -o errexit -o nounset; [ -n "${BASH:-}${ZSH_NAME:-}" ] && set -o pipefail


fileLocal="${1:-test.ts}"
prefix="${2:-}"
bucket="${3:-http://localhost:9000/ts-bucket/}"
region="${4:-cn-hz-xxlang}"
storageClass="STANDARD"


my_openssl() {
  if [ -f /usr/local/opt/openssl@1.1/bin/openssl ]; then
    /usr/local/opt/openssl@1.1/bin/openssl "$@"
  elif [ -f /usr/local/opt/openssl/bin/openssl ]; then
    /usr/local/opt/openssl/bin/openssl "$@"
  else
    openssl "$@"
  fi
}

my_sed() {
  if command -v gsed > /dev/null 2>&1; then
    gsed "$@"
  else
    sed "$@"
  fi
}


awsStringSign4() {
  kSecret="AWS4$1"
  kDate=$(printf         '%s' "$2" | my_openssl dgst -sha256 -hex -mac HMAC -macopt "key:${kSecret}"     2>/dev/null | my_sed 's/^.* //')
  kRegion=$(printf       '%s' "$3" | my_openssl dgst -sha256 -hex -mac HMAC -macopt "hexkey:${kDate}"    2>/dev/null | my_sed 's/^.* //')
  kService=$(printf      '%s' "$4" | my_openssl dgst -sha256 -hex -mac HMAC -macopt "hexkey:${kRegion}"  2>/dev/null | my_sed 's/^.* //')
  kSigning=$(printf 'aws4_request' | my_openssl dgst -sha256 -hex -mac HMAC -macopt "hexkey:${kService}" 2>/dev/null | my_sed 's/^.* //')
  signedString=$(printf  '%s' "$5" | my_openssl dgst -sha256 -hex -mac HMAC -macopt "hexkey:${kSigning}" 2>/dev/null | my_sed 's/^.* //')
  printf '%s' "${signedString}"
}


if [ -z "${AWS_ACCESS_KEY_ID:-}" ]; then
  >&2 echo '! AWS_ACCESS_KEY_ID envvars not set.'
  exit 1
fi
if [ -z "${AWS_SECRET_ACCESS_KEY:-}" ]; then
  >&2 echo '! AWS_SECRET_ACCESS_KEY envvars not set.'
  exit 1
fi
awsAccess="${AWS_ACCESS_KEY_ID}"
awsSecret="${AWS_SECRET_ACCESS_KEY}"


>&2 echo "! Uploading..." "${fileLocal}" "->" "${bucket}" ${prefix} "${region}" "${storageClass}"
>&2 echo "! | $(uname) | $(my_openssl version) | $(my_sed --version | head -1) |"


httpReq='PUT'
authType='AWS4-HMAC-SHA256'
service='s3'


if [ "${bucket#https://*}" != "${bucket}" ] || \
   [ "${bucket#http://*}" != "${bucket}" ]; then
  endpointUrl="${bucket}"
else
  endpointUrl="https://${bucket}.${service}.${region}.amazonaws.com/"
fi
dateNow=$(date -d now +%s)
fullUrl="${endpointUrl}${prefix}/$(date -u -d @${dateNow} +'%Y/%m/%d/%H/%M/%S.ts')"

hostport="$(printf '%s' "${fullUrl}" | sed -E -e 's|^https?://||g' -e 's|/.*$||')"
pathRemote="$(printf '%s' "${fullUrl}" | sed -E 's|^https?://||g' | grep -o -E '/.*$' | cut -c 2-)"

dateValueS="$(date -u -d @${dateNow} +'%Y%m%d')"
dateValueL="$(date -u -d @${dateNow} +'%Y%m%dT%H%M%SZ')"

if command -v file >/dev/null 2>&1; then
  contentType="$(file --brief --mime-type "${fileLocal}")"
else
  contentType='application/octet-stream'
fi


# 0. Hash the file to be uploaded
if [ -f "${fileLocal}" ]; then
  payloadHash=$(my_openssl dgst -sha256 -hex < "${fileLocal}" 2>/dev/null | my_sed 's/^.* //')
else
  >&2 echo "! File not found: '${fileLocal}'"
  exit 1
fi

# 1. Create canonical request
headerList='content-type;host;x-amz-content-sha256;x-amz-date;x-amz-storage-class'

canonicalRequest="\
${httpReq}
/${pathRemote}

content-type:${contentType}
host:${hostport}
x-amz-content-sha256:${payloadHash}
x-amz-date:${dateValueL}
x-amz-storage-class:${storageClass}

${headerList}
${payloadHash}"

canonicalRequestHash=$(printf '%s' "${canonicalRequest}" | my_openssl dgst -sha256 -hex 2>/dev/null | my_sed 's/^.* //')

# 2. Create string to sign
stringToSign="\
${authType}
${dateValueL}
${dateValueS}/${region}/${service}/aws4_request
${canonicalRequestHash}"

# 3. Sign the string
signature=$(awsStringSign4 "${awsSecret}" "${dateValueS}" "${region}" "${service}" "${stringToSign}")

# 4. Upload the file
if [ "${AWS_UPLOAD:-0}" -lt 1 ]; then
  >&2 echo '! AWS_UPLOAD envvars not set or less than 1.'
  exit 1
elif [ "${AWS_UPLOAD:-0}" -gt 1 ]; then
  logLevel="--verbose"
else
  logLevel="--silent"
fi
curl ${logLevel} --location --proto-redir =https --request "${httpReq}" --upload-file "${fileLocal}" \
  --header "Content-Type: ${contentType}" \
  --header "Host: ${hostport}" \
  --header "X-Amz-Content-SHA256: ${payloadHash}" \
  --header "X-Amz-Date: ${dateValueL}" \
  --header "X-Amz-Storage-Class: ${storageClass}" \
  --header "Authorization: ${authType} Credential=${awsAccess}/${dateValueS}/${region}/${service}/aws4_request, SignedHeaders=${headerList}, Signature=${signature}" \
  "${fullUrl}"

>&2 echo "! Uploaded"
