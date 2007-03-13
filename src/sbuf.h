/*
 * PgBouncer - Lightweight connection pooler for PostgreSQL.
 * 
 * Copyright (c) 2007 Marko Kreen, Skype Technologies OÜ
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

typedef enum {
	SBUF_EV_READ,
	SBUF_EV_RECV_FAILED,
	SBUF_EV_SEND_FAILED,
	SBUF_EV_CONNECT_FAILED,
	SBUF_EV_CONNECT_OK
} SBufEvent;

typedef struct SBuf SBuf;

/* callback should return true if it used one of sbuf_prepare_* on sbuf,
   false if it used sbuf_pause(), sbuf_close() or simply wants to wait for
   next event loop (eg. too few data available). */
typedef bool (*sbuf_proto_cb_t)(SBuf *sbuf,
				SBufEvent evtype,
				MBuf *mbuf,
				void *arg);

/* for some reason, libevent has no typedef for callback */
typedef void (*sbuf_libevent_cb)(int, short, void *);

struct SBuf {
	/* libevent handle */
	struct event ev;

	/* protocol callback function */
	sbuf_proto_cb_t proto_handler;
	void *arg;

	/* fd for this socket */
	int sock;

	/* dest SBuf for current packet */
	SBuf *dst;

	unsigned recv_pos;
	unsigned pkt_pos;
	unsigned pkt_remain;
	unsigned send_pos;
	unsigned send_remain;

	unsigned wait_send:1;
	unsigned pkt_skip:1;
	unsigned pkt_flush:1;
	unsigned is_unix:1;

	uint8 buf[0];
};

#define sbuf_socket(sbuf) ((sbuf)->sock)

void sbuf_init(SBuf *sbuf, sbuf_proto_cb_t proto_fn, void *arg);
void sbuf_accept(SBuf *sbuf, int read_sock, bool is_unix);
void sbuf_connect(SBuf *sbuf, const PgAddr *addr, int timeout_sec);

void sbuf_pause(SBuf *sbuf);
void sbuf_continue(SBuf *sbuf);
void sbuf_close(SBuf *sbuf);

/* proto_fn can use those functions to order behaviour */
void sbuf_prepare_send(SBuf *sbuf, SBuf *dst, unsigned amount, bool flush);
void sbuf_prepare_skip(SBuf *sbuf, int amount);

bool sbuf_answer(SBuf *sbuf, const void *buf, int len);

void sbuf_continue_with_callback(SBuf *sbuf, sbuf_libevent_cb cb);

static inline bool sbuf_empty(SBuf *sbuf)
{
	return sbuf->send_pos == sbuf->recv_pos
		&& sbuf->pkt_remain == 0;
}
