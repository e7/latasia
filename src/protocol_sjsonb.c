#include <arpa/inet.h>

#include "conf.h"
#include "protocol_sjsonb.h"

// 魔数要求各字节无重复，以便于快速搜索
#define MAGIC_NO                0xE78F8A9DU
#define PROTO_VER               1000


typedef struct __attribute__ ((packed)) {
    uint32_t magic_no;
    uint32_t proto_ver;
    uint16_t content_type; // 内容类型
    uint16_t content_ofst; // 内容偏移
    uint32_t content_len; // 内容长度
    uint32_t content_checksum; // 内容校验码
} sjsonb_header_t;
#define LEN_HEADER              sizeof(sjsonb_header_t)
// 最短json串长度
#define LEN_MIN_CONTENT         2
// 最大扩展区长度
#define MAX_EXTRA_LEN           64
// 最大载荷长度
#define MAX_CONT_LEN            (RBUF_SIZE - LEN_HEADER - MAX_EXTRA_LEN)


ssize_t lts_proto_sjsonb_encode_size(lts_str_t *content)
{
    return sizeof(sjsonb_header_t) + 0 + content->len;
}


void lts_proto_sjsonb_encode(
    lts_str_t *content, uint16_t content_type, lts_buffer_t *dst
)
{
    sjsonb_header_t header;
    uint16_t content_ofst = (uint16_t)(sizeof(sjsonb_header_t) + 0);

    // 包头初始化
    header.magic_no = htonl(MAGIC_NO);
    header.proto_ver = htonl(PROTO_VER);
    header.content_type = htons(content_type);
    header.content_ofst = htons(content_ofst);
    header.content_len = htonl(content->len);
    header.content_checksum = htonl(0);

    // 假定调用者分配了足够的缓冲
    ASSERT(0 == lts_buffer_append(dst, (uint8_t *)&header, sizeof(header)));
    ASSERT(0 == lts_buffer_append(dst, content->data, content->len));
    return;
}


int lts_proto_sjsonb_decode(
    lts_buffer_t *buf, uint16_t *content_type, lts_str_t *content
)
{
    int valid_header = FALSE;
    sjsonb_header_t header;
    uint32_t magic = htonl(MAGIC_NO);

    while (lts_buffer_pending(buf) >= sizeof(sjsonb_header_t)) {
        sjsonb_header_t *header_be;
        uint32_t *candi = (uint32_t *)buf->seek;

        if (*candi != magic) {
            buf->seek += sizeof(uint32_t);
            continue;
        }

        header_be = (sjsonb_header_t *)buf->seek;

        // 转换为本地字节序
        header.magic_no = MAGIC_NO;
        header.proto_ver = ntohl(header_be->proto_ver);
        header.content_type = ntohs(header_be->content_type);
        header.content_ofst = ntohs(header_be->content_ofst);
        header.content_len = ntohl(header_be->content_len);
        header.content_checksum = ntohl(header_be->content_checksum);

        if (PROTO_VER != header.proto_ver) {
            buf->seek += sizeof(uint32_t);
            continue;
        }

        if (header.content_ofst < sizeof(header)
            || header.content_ofst > MAX_EXTRA_LEN) {
            buf->seek += sizeof(uint32_t);
            continue;
        }

        if (header.content_len > MAX_CONT_LEN) {
            buf->seek += sizeof(uint32_t);
            continue;
        }

        if (0) { // verify checksum
            buf->seek += sizeof(uint32_t);
            continue;
        }

        valid_header = TRUE;
        break;
    }

    if (! valid_header) {
        return -1;
    }

    if (lts_buffer_pending(buf) < sizeof(header) + header.content_len) {
        // 内容长度不足
        return -1;
    }

    *content_type = header.content_type;
    content->data = buf->seek + header.content_ofst;
    content->len = header.content_len;

    buf->seek += header.content_ofst + header.content_len;

    return 0;
}
