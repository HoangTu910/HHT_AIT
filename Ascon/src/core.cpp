#include "core.h"

#include <string.h>

void ascon_duplex(state* s, unsigned char* out, const unsigned char* in,
                  unsigned long len, u8 mode) {
  u32_4 tmp;

  while (len >= RATE) {
    tmp.words[0].l = ((u32*)in)[0];
    tmp.words[0].h = ((u32*)in)[1];
    tmp.words[1].l = ((u32*)in)[2];
    tmp.words[1].h = ((u32*)in)[3];
    tmp = ascon_rev8(tmp);
    s->x0.h ^= tmp.words[0].h;
    s->x0.l ^= tmp.words[0].l;
    s->x1.h ^= tmp.words[1].h;
    s->x1.l ^= tmp.words[1].l;

    if (mode != ASCON_AD) {
      u32_4 tmp0 = ascon_rev8((u32_4){{s->x0, s->x1}});
      ((u32*)out)[0] = tmp0.words[0].l;
      ((u32*)out)[1] = tmp0.words[0].h;
      ((u32*)out)[2] = tmp0.words[1].l;
      ((u32*)out)[3] = tmp0.words[1].h;
    }
    if (mode == ASCON_DEC) {
      s->x0 = tmp.words[0];
      s->x1 = tmp.words[1];
    }

    P(s, PB_START_ROUND);

    in += RATE;
    out += RATE;
    len -= RATE;
  }

  u8* bytes = (u8*)&tmp;
  memset(bytes, 0, sizeof tmp);
  memcpy(bytes, in, len);
  bytes[len] ^= 0x80;

  tmp = ascon_rev8(tmp);
  s->x0.h ^= tmp.words[0].h;
  s->x0.l ^= tmp.words[0].l;
  s->x1.h ^= tmp.words[1].h;
  s->x1.l ^= tmp.words[1].l;

  if (mode != ASCON_AD) {
    tmp = ascon_rev8((u32_4){{s->x0, s->x1}});
    memcpy(out, bytes, len);
  }
  if (mode == ASCON_DEC) {
    memcpy(bytes, in, len);
    tmp = ascon_rev8(tmp);
    s->x0 = tmp.words[0];
    s->x1 = tmp.words[1];
  }
}

void process_data(state *s, unsigned char *out, const unsigned char *in, unsigned long long len, u8 mode)
{
    
}

void ascon_core(state *s, unsigned char *out, const unsigned char *in,
                unsigned long long tlen, const unsigned char *ad,
                unsigned long long adlen, const unsigned char *npub,
                const unsigned char *k, u8 mode)
{
    u32_4 tmp;
    u32_2 K0, K1, N0, N1;

    // load key
    tmp.words[0].l = ((u32 *)k)[0];
    tmp.words[0].h = ((u32 *)k)[1];
    tmp.words[1].l = ((u32 *)k)[2];
    tmp.words[1].h = ((u32 *)k)[3];
    tmp = ascon_rev8(tmp);
    K0 = tmp.words[0];
    K1 = tmp.words[1];

    // load nonce
    tmp.words[0].l = ((u32 *)npub)[0];
    tmp.words[0].h = ((u32 *)npub)[1];
    tmp.words[1].l = ((u32 *)npub)[2];
    tmp.words[1].h = ((u32 *)npub)[3];
    tmp = ascon_rev8(tmp);
    N0 = tmp.words[0];
    N1 = tmp.words[1];

    // initialization
    s->x0.h = IV;
    s->x0.l = 0;
    s->x1.h = K0.h;
    s->x1.l = K0.l;
    s->x2.h = K1.h;
    s->x2.l = K1.l;
    s->x3.h = N0.h;
    s->x3.l = N0.l;
    s->x4.h = N1.h;
    s->x4.l = N1.l;
    P(s, PA_START_ROUND);
    s->x3.h ^= K0.h;
    s->x3.l ^= K0.l;
    s->x4.h ^= K1.h;
    s->x4.l ^= K1.l;

    // process associated data
    if (adlen)
    {
        ascon_duplex(s, (unsigned char*)0, ad, adlen, ASCON_AD);
        P(s, PB_START_ROUND);
    }
    s->x4.l ^= 1;

    // process plaintext/ciphertext
    ascon_duplex(s, out, in, tlen, mode);

    // finalization
    s->x2.h ^= K0.h;
    s->x2.l ^= K0.l;
    s->x3.h ^= K1.h;
    s->x3.l ^= K1.l;
    P(s, PA_START_ROUND);
    s->x3.h ^= K0.h;
    s->x3.l ^= K0.l;
    s->x4.h ^= K1.h;
    s->x4.l ^= K1.l;
}

