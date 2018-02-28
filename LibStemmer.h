#ifndef LIBSTEMMER_H
#define LIBSTEMMER_H

/*
  Copyright (c) 2001, Dr Martin Porter,
  Copyright (c) 2002, Richard Boulton.
  Copyright 2016 Dr Rochus Keller
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted provided
  that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the
	 following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	 the following disclaimer in the documentation and/or other materials provided with the distribution.

  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or
	 promote products derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
  OF SUCH DAMAGE.
*/

// adaptiert aus libstemmer http://snowball.tartarus.org/

typedef char symbol; // original ist unsigned

struct SN_env {
	symbol * p;
	int c; int l; int lb; int bra; int ket;
	symbol * * S;
	int * I;
	unsigned char * B;
};
struct among
{   int s_size;     /* number of chars in string */
	const symbol * s;       /* search string */
	int substring_i;/* index to longest matching substring */
	int result;     /* result of the lookup */
	int (* function)(struct SN_env *);
};
#define HEAD 2*sizeof(int)

#define SIZE(p)        ((int *)(p))[-1]
#define SET_SIZE(p, n) ((int *)(p))[-1] = n
#define CAPACITY(p)    ((int *)(p))[-2]

extern int eq_s(struct SN_env * z, int s_size, const symbol * s);
extern int slice_from_s(struct SN_env * z, int s_size, const symbol * s);
extern int in_grouping(struct SN_env * z, const unsigned char * s, int min, int max, int repeat);
extern int out_grouping(struct SN_env * z, const unsigned char * s, int min, int max, int repeat);
extern int find_among(struct SN_env * z, const struct among * v, int v_size);
extern int find_among_b(struct SN_env * z, const struct among * v, int v_size);
extern int slice_del(struct SN_env * z);
extern int eq_s_b(struct SN_env * z, int s_size, const symbol * s);
extern int in_grouping_b(struct SN_env * z, const unsigned char * s, int min, int max, int repeat);
extern struct SN_env * SN_create_env(int S_size, int I_size, int B_size);
extern void SN_close_env(struct SN_env * z, int S_size);

#endif // LIBSTEMMER_H
