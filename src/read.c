/*
   Graph Plotter for numerical data analysis.
   Copyright (C) 2022 Roman Belov <romblv@gmail.com>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <SDL2/SDL.h>

#ifdef _WINDOWS
#include <windows.h>
#endif /* _WINDOWS */

#include "async.h"
#include "dirent.h"
#include "lang.h"
#include "plot.h"
#include "read.h"

int utf8_length(const char *s);
const char *utf8_skip(const char *s, int n);
const char *utf8_skip_b(const char *s, int n);

static char *
stoi(const markup_t *mk, int *x, char *s)
{
	int		n, k, i;

	if (*s == '-') { n = - 1; s++; }
	else if (*s == '+') { n = 1; s++; }
	else { n = 1; }

	k = 0;
	i = 0;

	while (*s >= '0' && *s <= '9') {

		i = 10 * i + (*s++ - '0') * n;
		k++;

		if (k > 9) return NULL;
	}

	if (k == 0) return NULL;

	if (*s == 0 || strchr(mk->space, *s) != NULL || strchr(mk->lend, *s) != NULL) {

		*x = i;
	}
	else return NULL;

	return s;
}

static char *
htoi(const markup_t *mk, int *x, char *s)
{
	int		i, k, h;

	if (*s == '0' && *(s + 1) == 'x') { s += 2; }

	k = 0;
	h = 0;

	do {
		if (*s >= '0' && *s <= '9') { i = *s++ - '0'; }
		else if (*s >= 'A' && *s <= 'F') { i = 10 + *s++ - 'A'; }
		else if (*s >= 'a' && *s <= 'f') { i = 10 + *s++ - 'a'; }
		else break;

		h = 16 * h + i;
		k++;

		if (k > 8) return NULL;
	}
	while (1);

	if (k == 0) return NULL;

	if (*s == 0 || strchr(mk->space, *s) != NULL || strchr(mk->lend, *s) != NULL) {

		*x = h;
	}
	else return NULL;

	return s;
}

static char *
otoi(const markup_t *mk, int *x, char *s)
{
	int		i, k, h;

	k = 0;
	h = 0;

	do {
		if (*s >= '0' && *s <= '7') { i = *s++ - '0'; }
		else break;

		h = 8 * h + i;
		k++;

		if (k > 11) return NULL;
	}
	while (1);

	if (k == 0) return NULL;

	if (*s == 0 || strchr(mk->space, *s) != NULL || strchr(mk->lend, *s) != NULL) {

		*x = h;
	}
	else return NULL;

	return s;
}

static char *
stod(const markup_t *mk, double *x, char *s)
{
	int		n, k, v, e;
	double		f = 0.;

	if (*s == '-') { n = - 1; s++; }
	else if (*s == '+') { n = 1; s++; }
	else { n = 1; }

	k = 0;
	v = 0;
	f = 0.;

	while (*s >= '0' && *s <= '9') {

		f = 10. * f + (*s++ - '0') * n;
		k++;
	}

	if (*s == mk->delim) {

		s++;

		while (*s >= '0' && *s <= '9') {

			f = 10. * f + (*s++ - '0') * n;
			k++; v--;
		}
	}

	if (k == 0) return NULL;

	if (*s == 'n') { v += - 9; s++; }
	else if (*s == 'u') { v += - 6; s++; }
	else if (*s == 'm') { v += - 3; s++; }
	else if (*s == 'K') { v += 3; s++; }
	else if (*s == 'M') { v += 6; s++; }
	else if (*s == 'G') { v += 9; s++; }
	else if (*s == 'e' || *s == 'E') {

		s = stoi(mk, &e, s + 1);

		if (s != NULL) { v += e; }
		else return NULL;
	}

	if (*s == 0 || strchr(mk->space, *s) != NULL || strchr(mk->lend, *s) != NULL) {

		while (v < 0) { f /= 10.; v++; }
		while (v > 0) { f *= 10.; v--; }

		*x = f;
	}
	else return NULL;

	return s;
}

read_t *readAlloc(plot_t *pl)
{
	read_t		*rd;

	rd = calloc(1, sizeof(read_t));

	rd->pl = pl;

	strcpy(rd->screenpath, ".");

	rd->window_size_x = GP_MIN_SIZE_X;
	rd->window_size_y = GP_MIN_SIZE_Y;
	rd->antialiasing = DRAW_4X_MSAA;
	rd->solidfont = 0;
	rd->thickness = 1;
	rd->timecol = -1;
	rd->shortfilename = 0;

	rd->mk_config.delim = '.';
	strcpy(rd->mk_config.space, " \t;");
	strcpy(rd->mk_config.lend, "\r\n");

	rd->mk_text.delim = rd->mk_config.delim;
	strcpy(rd->mk_text.space, rd->mk_config.space);
	strcpy(rd->mk_text.lend, rd->mk_config.lend);

#ifdef _WINDOWS
	rd->legacy_label_enc = 0;
#endif /* _WINDOWS */

	rd->preload = 8388608;
	rd->chunk = 4096;
	rd->timeout = 10000;
	rd->length_N = 10000;

	rd->bind_N = -1;
	rd->page_N = -1;
	rd->figure_N = -1;

	return rd;
}

void readClean(read_t *rd)
{
	free(rd);
}

#ifdef _WINDOWS
void legacy_ACP_to_UTF8(char *ustr, const char *text, int n)
{
	wchar_t			wbuf[READ_TOKEN_MAX * READ_COLUMN_MAX];

	MultiByteToWideChar(CP_ACP, 0, text, -1, wbuf, sizeof(wbuf) / sizeof(wchar_t));
	WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, ustr, n, NULL, NULL);
}

void legacy_OEM_to_UTF8(char *ustr, const char *text, int n)
{
	wchar_t			wbuf[READ_TOKEN_MAX * READ_COLUMN_MAX];

	MultiByteToWideChar(CP_OEMCP, 0, text, -1, wbuf, sizeof(wbuf) / sizeof(wchar_t));
	WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, ustr, n, NULL, NULL);
}

static FILE *
legacy_fopen_from_UTF8(const char *file, const char *mode)
{
	wchar_t			wfile[READ_FILE_PATH_MAX];
	wchar_t			wmode[READ_TOKEN_MAX];

	MultiByteToWideChar(CP_UTF8, 0, file, -1, wfile, READ_FILE_PATH_MAX);
	MultiByteToWideChar(CP_UTF8, 0, mode, -1, wmode, READ_TOKEN_MAX);

	return _wfopen(wfile, wmode);
}

static FILE *
legacy_fopen_from_ACP(const char *file, const char *mode)
{
	wchar_t			wfile[READ_FILE_PATH_MAX];
	wchar_t			wmode[READ_TOKEN_MAX];

	MultiByteToWideChar(CP_ACP, 0, file, -1, wfile, READ_FILE_PATH_MAX);
	MultiByteToWideChar(CP_ACP, 0, mode, -1, wmode, READ_TOKEN_MAX);

	return _wfopen(wfile, wmode);
}

static char *
legacy_trim(read_t *rd, char *s)
{
	char		*eol;

	while (*s != 0) {

		if (*s != '\'' && *s != ' ')
			break;

		s++;
	}

	if (strlen(s) > 1) {

		eol = s + strlen(s) - 1;

		if (strchr(rd->mk_config.lend, *eol) != NULL) {

			*eol = 0;
		}
	}

	return s;
}

void legacy_readConfigGRM(read_t *rd, const char *confile, const char *file)
{
	FILE		*fd;
	char 		*sbuf = rd->data[0].buf;

	int		n, cX, cY, cYm;
	double		scale, offset;

	int		stub, cN, line_N;
	float		fpN;

	fd = legacy_fopen_from_ACP(file, "rb");

	if (fd == NULL) {

		ERROR("fopen(\"%s\"): %s\n", file, strerror(errno));
	}
	else {
		fread(&stub, 2, 1, fd);
		fread(&fpN, 4, 1, fd);

		cN = (int) fpN;

		if (fpN == (float) cN && cN > 1 && cN < READ_COLUMN_MAX) {

			fclose(fd);

			readOpenUnified(rd, 0, cN + 1, 0, file, FORMAT_BINARY_LEGACY_V1);
		}
		else {
			fseek(fd, 0UL, SEEK_SET);
			fread(&fpN, 4, 1, fd);

			cN = (int) fpN;

			if (fpN == (float) cN && cN > 1 && cN < READ_COLUMN_MAX) {

				fclose(fd);

				readOpenUnified(rd, 0, cN + 1, 0, file, FORMAT_BINARY_LEGACY_V2);
			}
			else {
				fclose(fd);

				ERROR("Unable to load legacy file \"%s\"\n", file);
			}
		}
	}

	fd = legacy_fopen_from_ACP(confile, "r");

	if (fd == NULL) {

		ERROR("fopen(\"%s\"): %s\n", confile, strerror(errno));
	}
	else if (rd->data[0].file[0] != 0) {

		line_N = 0;

		rd->page_N = 0;

		while (fgets(sbuf, sizeof(rd->data[0].buf), fd) != NULL) {

			line_N++;

			if (strncmp(sbuf, "LI", 2) == 0) {

				fgets(sbuf, sizeof(rd->data[0].buf), fd);

				line_N++;

				legacy_OEM_to_UTF8(sbuf, sbuf, sizeof(rd->data[0].buf));
				sbuf[READ_TOKEN_MAX] = 0;

				rd->figure_N = -1;
				rd->page_N++;

				rd->page[rd->page_N].busy = 1;
				strcpy(rd->page[rd->page_N].title, legacy_trim(rd, sbuf));

				fgets(sbuf, sizeof(rd->data[0].buf), fd);

				line_N++;

				n = sscanf(sbuf, "%i", &cX);

				if (n != 1) {

					cX = 0;
				}

				if (cX < -1 || cX >= rd->pl->data[0].column_N) {

					ERROR("%s:%i: page %i column number %i is out of range\n",
						confile, line_N, rd->page_N, cX);
					cX = 0;
				}

				fgets(sbuf, sizeof(rd->data[0].buf), fd);

				line_N++;

				legacy_OEM_to_UTF8(sbuf, sbuf, sizeof(rd->data[0].buf));
				sbuf[READ_TOKEN_MAX] = 0;

				strcpy(rd->page[rd->page_N].ax[0].label, legacy_trim(rd, sbuf));

				do {
					fgets(sbuf, sizeof(rd->data[0].buf), fd);

					line_N++;

					if (strlen(sbuf) < 5) {

						break;
					}

					n = sscanf(sbuf, "%*s %i %i %le %le",
							&cY, &cYm, &scale, &offset);

					if (n != 4) {

						ERROR("%s:%i: page %i figure %i invalid format\n",
							confile, line_N, rd->page_N, rd->figure_N + 1);
						break;
					}

					if (cY < -1 || cY >= rd->pl->data[0].column_N) {

						ERROR("%s:%i: page %i column number %i is out of range\n",
							confile, line_N, rd->page_N, cY);
						break;
					}

					rd->figure_N++;

					rd->page[rd->page_N].fig[rd->figure_N].busy = 1;
					rd->page[rd->page_N].fig[rd->figure_N].drawing = -1;
					rd->page[rd->page_N].fig[rd->figure_N].dN = 0;
					rd->page[rd->page_N].fig[rd->figure_N].cX = cX;
					rd->page[rd->page_N].fig[rd->figure_N].cY = cY;
					rd->page[rd->page_N].fig[rd->figure_N].aX = 0;
					rd->page[rd->page_N].fig[rd->figure_N].aY = 1;

					sprintf(rd->page[rd->page_N].fig[rd->figure_N].label,
							"fig.%i.%i", rd->figure_N, cY);

					if (scale != 1. || offset != 0.) {

						rd->page[rd->page_N].fig[rd->figure_N].ops[1].busy = SUBTRACT_SCALE;
						rd->page[rd->page_N].fig[rd->figure_N].ops[1].scale = scale;
						rd->page[rd->page_N].fig[rd->figure_N].ops[1].offset = offset;
					}

					if (cY != cYm) {

						rd->page[rd->page_N].fig[rd->figure_N].ops[1].busy = SUBTRACT_BINARY_SUBTRACTION;
						rd->page[rd->page_N].fig[rd->figure_N].ops[1].column_2 = cYm;
						rd->page[rd->page_N].fig[rd->figure_N].ops[1].scale = scale;
						rd->page[rd->page_N].fig[rd->figure_N].ops[1].offset = offset;
					}

					if (strstr(sbuf, "'END") != NULL)
						break;

					if (rd->figure_N >= PLOT_FIGURE_MAX) {

						ERROR("%s:%i: too many figures on page %i\n",
							confile, line_N, rd->page_N);
						break;
					}
				}
				while (1);
			}
		}

		fclose(fd);
	}
}
#endif /* _WINDOWS */

FILE *unified_fopen(const char *file, const char *mode)
{
#ifdef _WINDOWS
	return legacy_fopen_from_UTF8(file, mode);
#else /* _WINDOWS */
	return fopen(file, mode);
#endif
}

static int
TEXT_GetRow(read_t *rd, int dN)
{
	fval_t 		*row = rd->data[dN].row;
	int		*hint = rd->data[dN].hint;
	char 		*r, *s = rd->data[dN].buf;
	int		hex, N = 0, m = 0;
	double		val;

	while (*s != 0) {

		if (strchr(rd->mk_text.space, *s) != NULL || strchr(rd->mk_text.lend, *s) != NULL) {

			m = 0;
		}
		else {
			if (m == 0) {

				m = 1;

				if (hint[N] == DATA_HINT_FLOAT) {

					r = stod(&rd->mk_text, &val, s);

					if (r != NULL) {

						*row++ = (fval_t) val;
					}
					else {
						*row++ = (fval_t) FP_NAN;
					}
				}
				else if (hint[N] == DATA_HINT_HEX) {

					r = htoi(&rd->mk_text, &hex, s);

					if (r != NULL) {

						*row++ = (fval_t) hex;
					}
					else {
						*row++ = (fval_t) FP_NAN;
					}
				}
				else if (hint[N] == DATA_HINT_OCT) {

					r = otoi(&rd->mk_text, &hex, s);

					if (r != NULL) {

						*row++ = (fval_t) hex;
					}
					else {
						*row++ = (fval_t) FP_NAN;
					}
				}
				else {
					r = stod(&rd->mk_text, &val, s);

					if (r != NULL) {

						*row++ = (fval_t) val;
					}
					else {
						r = htoi(&rd->mk_text, &hex, s);

						if (r != NULL) {

							if (hint[N] == DATA_HINT_NONE) {

								hint[N] = DATA_HINT_HEX;
							}

							*row++ = (fval_t) hex;
						}
						else {
							*row++ = (fval_t) FP_NAN;
						}
					}
				}

				N++;

				if (N >= READ_COLUMN_MAX)
					break;
			}
		}

		s++;
	}

	return N;
}

static int
TEXT_GetLabel(read_t *rd, int dN)
{
	char		*label, *s = rd->data[dN].buf;
	int		N = 0, m = 0;

#ifdef _WINDOWS
	if (rd->legacy_label_enc == 1) {

		legacy_ACP_to_UTF8(s, s, sizeof(rd->data[dN].buf));
	}
	else if (rd->legacy_label_enc == 2) {

		legacy_OEM_to_UTF8(s, s, sizeof(rd->data[dN].buf));
	}
#endif /* _WINDOWS */

	while (*s != 0) {

		if (strchr(rd->mk_text.space, *s) != NULL || strchr(rd->mk_text.lend, *s) != NULL) {

			if (m != 0) {

				*label = 0;
				m = 0;
			}
		}
		else {
			if (m == 0) {

				rd->data[dN].hint[N] = DATA_HINT_NONE;
				label = rd->data[dN].label[N++];

				if (N >= READ_COLUMN_MAX)
					break;
			}

			if (m < READ_TOKEN_MAX - 1) {

				*label++ = *s;
				m++;
			}
		}

		s++;
	}

	if (m != 0) {

		*label = 0;
	}

	return N;
}

static char *
follow_fgets(char *s, int len, FILE *fd, int timeout)
{
	int		c, eol, nq, waiting;

	eol = 0;
	nq = 0;
	waiting = 0;

	do {
		c = fgetc(fd);

		if (c != EOF) {

			if (c == '\r' || c == '\n') {

				eol = (nq > 0) ? 1 : 0;
			}
			else if (eol == 1) {

				ungetc(c, fd);
				break;
			}
			else if (nq < len - 1) {

				*s++ = (char) c;
				nq++;
			}
		}
		else {
			if (feof(fd) || ferror(fd)) {

				if (waiting < timeout) {

					clearerr(fd);

					SDL_Delay(10);

					waiting += 10;
				}
				else {
					break;
				}
			}
		}
	}
	while (1);

	if (eol != 0) {

		*s = 0;

		return s;
	}
	else {
		return NULL;
	}
}

static int
TEXT_GetCN(read_t *rd, int dN, FILE *fd, fval_t *rbuf)
{
	int		label_cN, fixed_N, total_N;
	int		N, cN, timeout;
	char		*r;

	label_cN = 0;
	cN = 0;

	fixed_N = 0;
	total_N = 0;

	timeout = (rd->data[dN].follow != 0) ? rd->timeout : 0;

	do {
		r = follow_fgets(rd->data[dN].buf, sizeof(rd->data[0].buf), fd, timeout);

		total_N++;

		if (r == NULL)
			break;

		if (label_cN < 1) {

			label_cN = TEXT_GetLabel(rd, dN);
		}
		else {
			cN = TEXT_GetRow(rd, dN);

			if (cN != 0) {

				if (cN != label_cN) {

					label_cN = TEXT_GetLabel(rd, dN);
					fixed_N = 0;
				}
				else {
					for (N = 0; N < cN; ++N)
						rbuf[fixed_N * READ_COLUMN_MAX + N] = rd->data[dN].row[N];

					fixed_N++;

					if (fixed_N >= 3) {

						rd->data[dN].line_N = total_N + 1;
						break;
					}
				}
			}
		}

		if (total_N >= READ_TEXT_HEADER_MAX) {

			cN = 0;
			break;
		}
	}
	while (1);

	return cN;
}

static ulen_t
FILE_GetSize(const char *file)
{
	unsigned long long	sb;

	if (fstatsize(file, &sb) != 0) {

		sb = 0U;
	}

	return (ulen_t) sb;
}

static void
readClose(read_t *rd, int dN)
{
	async_close(rd->data[dN].afd);

	if (rd->data[dN].fd != stdin) {

		fclose(rd->data[dN].fd);
	}

	rd->data[dN].fd = NULL;
	rd->data[dN].afd = NULL;

	rd->files_N -= 1;
}

void readOpenUnified(read_t *rd, int dN, int cN, int lN, const char *file, int fmt)
{
	fval_t		rbuf[READ_COLUMN_MAX * 3];
	FILE		*fd;
	ulen_t		sF = 0U;

	if (rd->data[dN].fd != NULL) {

		readClose(rd, dN);
	}

	if (file != NULL) {

		fd = unified_fopen(file, "rb");
	}
	else if (fmt == FORMAT_PLAIN_TEXT) {

		fd = stdin;
	}

	if (fd == NULL) {

		ERROR("fopen(\"%s\"): %s\n", file, strerror(errno));
	}
	else {
		if (file != NULL) {

			sF = FILE_GetSize(file);
		}

		if (rd->data[dN].follow != 0 && sF != 0) {

			fseek(fd, 0UL, SEEK_END);
		}

		rd->data[dN].length_N = lN;

		if (fmt == FORMAT_PLAIN_TEXT) {

			cN = TEXT_GetCN(rd, dN, fd, rbuf);

			if (cN < 1) {

				ERROR("No correct data in file \"%s\"\n",
						(file != NULL) ? file : "STDIN");
				fclose(fd);
				return ;
			}

			if (sF != 0 && lN < 1) {

				/* We do not use the file size to guess the
				 * length since there is an incremental dataset
				 * memory allocation.
				 * */
				lN = 1000;
			}
			else if (lN < 1) {

				lN = rd->length_N;
			}
		}
		else if (fmt == FORMAT_BINARY_FLOAT) {

			lN = (lN < 1) ? sF / (cN * sizeof(float)) : lN;
			rd->data[dN].line_N = 1;
		}
		else if (fmt == FORMAT_BINARY_DOUBLE) {

			lN = (lN < 1) ? sF / (cN * sizeof(double)) : lN;
			rd->data[dN].line_N = 1;
		}

#ifdef _WINDOWS
		else if (fmt == FORMAT_BINARY_LEGACY_V1) {

			lN = (lN < 1) ? (sF - 6) / (cN * 6) : lN;
			rd->data[dN].line_N = 1;

			fseek(fd, 6UL, SEEK_SET);
		}
		else if (fmt == FORMAT_BINARY_LEGACY_V2) {

			lN = (lN < 1) ? (sF - 4) / (cN * 4) : lN;
			rd->data[dN].line_N = 1;

			fseek(fd, 4UL, SEEK_SET);
		}
#endif /* _WINDOWS */

		plotDataAlloc(rd->pl, dN, cN, lN + 1);

		if (fmt == FORMAT_PLAIN_TEXT) {

			plotDataInsert(rd->pl, dN, rbuf + READ_COLUMN_MAX * 0);
			plotDataInsert(rd->pl, dN, rbuf + READ_COLUMN_MAX * 1);
			plotDataInsert(rd->pl, dN, rbuf + READ_COLUMN_MAX * 2);
		}

		rd->data[dN].format = fmt;
		rd->data[dN].column_N = cN;

		strcpy(rd->data[dN].file, (file != NULL) ? file : "STDIN");

		rd->data[dN].fd = fd;
		rd->data[dN].afd = async_open(fd, rd->preload, rd->chunk, rd->timeout);

		rd->files_N += 1;
		rd->bind_N = dN;
	}
}

void readOpenStub(read_t *rd, int dN, int cN, int lN, const char *file, int fmt)
{
	rd->data[dN].length_N = lN;

	lN = (lN < 1) ? 10 : lN;

	plotDataAlloc(rd->pl, dN, cN, lN + 1);

	rd->data[dN].format = fmt;
	rd->data[dN].column_N = cN;

	strcpy(rd->data[dN].file, file);

	rd->bind_N = dN;
}

void readToggleHint(read_t *rd, int dN, int cN)
{
	if (rd->data[dN].format == FORMAT_NONE) {

		ERROR("Dataset number %i was not allocated\n", dN);
		return ;
	}

	if (cN < 0 || cN >= rd->data[dN].column_N) {

		return ;
	}

	if (rd->data[dN].hint[cN] == DATA_HINT_NONE) {

		rd->data[dN].hint[cN] = DATA_HINT_FLOAT;
	}
	else if (rd->data[dN].hint[cN] == DATA_HINT_FLOAT) {

		rd->data[dN].hint[cN] = DATA_HINT_HEX;
	}
	else if (rd->data[dN].hint[cN] == DATA_HINT_HEX) {

		rd->data[dN].hint[cN] = DATA_HINT_OCT;
	}
	else if (rd->data[dN].hint[cN] == DATA_HINT_OCT) {

		rd->data[dN].hint[cN] = DATA_HINT_NONE;
	}
}

static int
TEXT_Read(read_t *rd, int dN)
{
	int		r, cN;

	r = async_gets(rd->data[dN].afd, rd->data[dN].buf, sizeof(rd->data[0].buf));

	if (r == ASYNC_OK) {

		cN = TEXT_GetRow(rd, dN);

		if (cN == rd->pl->data[dN].column_N) {

			plotDataInsert(rd->pl, dN, rd->data[dN].row);
		}

		return 1;
	}
	else if (r == ASYNC_END_OF_FILE) {

		readClose(rd, dN);
	}

	return 0;
}

static int
FLOAT_Read(read_t *rd, int dN)
{
	float		*fb = (float *) rd->data[dN].buf;
	int		r, N, cN = rd->pl->data[dN].column_N;

	r = async_read(rd->data[dN].afd, (void *) fb, cN * sizeof(float));

	if (r == ASYNC_OK) {

		for (N = 0; N < cN; ++N)
			rd->data[dN].row[N] = (fval_t) fb[N];

		plotDataInsert(rd->pl, dN, rd->data[dN].row);

		return 1;
	}
	else if (r == ASYNC_END_OF_FILE) {

		readClose(rd, dN);
	}

	return 0;
}

static int
DOUBLE_Read(read_t *rd, int dN)
{
	double		*fb = (double *) rd->data[dN].buf;
	int		r, N, cN = rd->pl->data[dN].column_N;

	r = async_read(rd->data[dN].afd, (void *) fb, cN * sizeof(double));

	if (r == ASYNC_OK) {

		for (N = 0; N < cN; ++N)
			rd->data[dN].row[N] = (fval_t) fb[N];

		plotDataInsert(rd->pl, dN, rd->data[dN].row);

		return 1;
	}
	else if (r == ASYNC_END_OF_FILE) {

		readClose(rd, dN);
	}

	return 0;
}

#ifdef _WINDOWS
static int
LEGACY_Read(read_t *rd, int dN)
{
	char		*fb = (char *) rd->data[dN].buf;
	int		r, N, cN = rd->pl->data[dN].column_N;

	if (rd->data[dN].format == FORMAT_BINARY_LEGACY_V1) {

		r = async_read(rd->data[dN].afd, (void *) fb, cN * 6);
	}
	else {
		r = async_read(rd->data[dN].afd, (void *) fb, cN * 4);
	}

	if (r == ASYNC_OK) {

		if (rd->data[dN].format == FORMAT_BINARY_LEGACY_V1) {

			for (N = 0; N < cN; ++N)
				rd->data[dN].row[N] = * (float *) (fb + N * 6 + 2);
		}
		else {
			for (N = 0; N < cN; ++N)
				rd->data[dN].row[N] = * (float *) (fb + N * 4);
		}

		plotDataInsert(rd->pl, dN, rd->data[dN].row);

		return 1;
	}
	else if (r == ASYNC_END_OF_FILE) {

		readClose(rd, dN);
	}

	return 0;
}
#endif /* _WINDOWS */

int readUpdate(read_t *rd)
{
	FILE		*fd;
	int		dN, bN;
	int		file_N = 0;
	int		ulN = 0;
	int		tTOP;

	for (dN = 0; dN < PLOT_DATASET_MAX; ++dN) {

		fd = rd->data[dN].fd;

		if (fd != NULL) {

			bN = 0;
			file_N += 1;

			tTOP = SDL_GetTicks() + 20;

			do {
				if (rd->data[dN].format == FORMAT_PLAIN_TEXT) {

					if (TEXT_Read(rd, dN) != 0) {

						ulN += 1;
					}
					else {
						break;
					}
				}
				else if (rd->data[dN].format == FORMAT_BINARY_FLOAT) {

					if (FLOAT_Read(rd, dN) != 0) {

						ulN += 1;
					}
					else {
						break;
					}
				}
				else if (rd->data[dN].format == FORMAT_BINARY_DOUBLE) {

					if (DOUBLE_Read(rd, dN) != 0) {

						ulN += 1;
					}
					else {
						break;
					}
				}

#ifdef _WINDOWS
				else if (rd->data[dN].format == FORMAT_BINARY_LEGACY_V1
					|| rd->data[dN].format == FORMAT_BINARY_LEGACY_V2) {

					if (LEGACY_Read(rd, dN) != 0) {

						ulN += 1;
					}
					else {
						break;
					}
				}
#endif /* _WINDOWS */

				rd->data[dN].line_N++;
				bN++;

				if (		rd->data[dN].length_N < 1
						&& plotDataSpaceLeft(rd->pl, dN) < 10) {

					plotDataGrowUp(rd->pl, dN);
				}
			}
			while (SDL_GetTicks() < tTOP);

			plotDataSubtract(rd->pl, dN, -1);
		}
	}

	rd->files_N = (file_N < rd->files_N) ? file_N : rd->files_N;

	return ulN;
}

static int
config_getc(parse_t *pa)
{
	int		r;

	if (pa->unchar < 0) {

		r = fgetc(pa->fd);
		r = (r == EOF) ? -1 : r;
	}
	else {
		r = pa->unchar;
		pa->unchar = -1;
	}

	return r;
}

static int
config_ungetc(parse_t *pa, int c)
{
	pa->unchar = c;

	return c;
}

static int
configLexerFSM(read_t *rd, parse_t *pa)
{
	char		*p = pa->tbuf;
	int		c, n = 0, r = 0;

	do { c = config_getc(pa); }
	while (strchr(rd->mk_config.space, c) != NULL);

	if (c == -1) {

		r = -1;
	}
	else if (c == '"') {

		c = config_getc(pa);
		n = 0;

		while (c != -1 && c != '"' && strchr(rd->mk_config.lend, c) == NULL) {

			if (n < READ_TOKEN_MAX - 1) {

				*p++ = c;
				n++;
			}

			c = config_getc(pa);
		}

		if (strchr(rd->mk_config.lend, c) != NULL)
			config_ungetc(pa, c);

		*p = 0;
	}
	else if (strchr(rd->mk_config.lend, c) != NULL) {

		r = 1;
		pa->line_N++;
		pa->newline = 1;
	}
	else {
		do {
			if (n < READ_TOKEN_MAX) {

				*p++ = c;
				n++;
			}

			c = config_getc(pa);
		}
		while (c != -1 && strchr(rd->mk_config.space, c) == NULL
			&& strchr(rd->mk_config.lend, c) == NULL);

		if (strchr(rd->mk_config.lend, c) != NULL)
			config_ungetc(pa, c);

		*p = 0;
	}

	return r;
}

static void
configParseFSM(read_t *rd, parse_t *pa)
{
	char		msg_tbuf[READ_FILE_PATH_MAX];
	char		*tbuf = pa->tbuf;
	int		r, failed;

	double		argd[2];
	int		argi[4];

	int		flag_follow;
	int		flag_stub;

	FILE		*fd;
	parse_t		rpa;

	failed = 0;

	do {
		r = configLexerFSM(rd, pa);

		if (r == -1) break;
		else if (r == 1) {

			failed = 0;
			continue;
		}
		else if (r == 0 && pa->newline != 0) {

			pa->newline = 0;

			sprintf(msg_tbuf, "unable to parse \"%.80s\"", tbuf);

			if (tbuf[0] == '#') {

				failed = 0;
				while (configLexerFSM(rd, pa) == 0) ;
			}
			else if (strcmp(tbuf, "include") == 0) {

				r = configLexerFSM(rd, pa);

				if (r == 0) {

					fd = unified_fopen(tbuf, "r");

					if (fd == NULL) {

						ERROR("fopen(\"%s\"): %s\n",
								tbuf, strerror(errno));
						failed = 1;
					}
					else {
						strcpy(rpa.file, tbuf);

						rpa.fd = fd;
						rpa.unchar = -1;
						rpa.line_N = 1;
						rpa.newline = 1;

						configParseFSM(rd, &rpa);

						fclose(fd);
					}
				}
				else {
					failed = 1;
				}
			}
			else if (strcmp(tbuf, "font") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					r = configLexerFSM(rd, pa);

					if (r == 0) {

						if (strcmp(tbuf, "normal") == 0) {

							plotFontDefault(rd->pl, TTF_ID_ROBOTO_MONO_NORMAL,
									argi[0], TTF_STYLE_NORMAL);
						}
						else if (strcmp(tbuf, "normal-bold") == 0) {

							plotFontDefault(rd->pl, TTF_ID_ROBOTO_MONO_NORMAL,
									argi[0], TTF_STYLE_BOLD);
						}
						else if (strcmp(tbuf, "normal-italic") == 0) {

							plotFontDefault(rd->pl, TTF_ID_ROBOTO_MONO_NORMAL,
									argi[0], TTF_STYLE_ITALIC);
						}
						else if (strcmp(tbuf, "thin") == 0) {

							plotFontDefault(rd->pl, TTF_ID_ROBOTO_MONO_THIN,
									argi[0], TTF_STYLE_NORMAL);
						}
						else if (strcmp(tbuf, "thin-bold") == 0) {

							plotFontDefault(rd->pl, TTF_ID_ROBOTO_MONO_THIN,
									argi[0], TTF_STYLE_BOLD);
						}
						else if (strcmp(tbuf, "thin-italic") == 0) {

							plotFontDefault(rd->pl, TTF_ID_ROBOTO_MONO_THIN,
									argi[0], TTF_STYLE_ITALIC);
						}
						else {
							plotFontOpen(rd->pl, tbuf, argi[0], TTF_STYLE_NORMAL);
						}

						if (rd->pl->font != NULL) {

							failed = 0;
						}
					}
				}
				while (0);
			}

#ifdef _WINDOWS
			else if (strcmp(tbuf, "legacy_label_enc") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					failed = 0;
					rd->legacy_label_enc = argi[0];
				}
				while (0);
			}
#endif /* _WINDOWS */

			else if (strcmp(tbuf, "preload") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					if (argi[0] > sizeof(rd->data[0].buf)) {

						failed = 0;
						rd->preload = argi[0];
					}
					else {
						sprintf(msg_tbuf, "preload size %i is too small", argi[0]);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "chunk") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					if (argi[0] > 0) {

						failed = 0;
						rd->chunk = argi[0];
					}
					else {
						sprintf(msg_tbuf, "chunk size %i must be positive", argi[0]);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "timeout") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					if (argi[0] >= 0) {

						failed = 0;
						rd->timeout = argi[0];
					}
					else {
						sprintf(msg_tbuf, "timeout %i must be non-negative", argi[0]);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "batch") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					if (argi[0] > 0) {

						/* FIXME: ignore legacy */

						failed = 0;
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "length") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					if (argi[0] > 0) {

						failed = 0;
						rd->length_N = argi[0];
					}
					else {
						sprintf(msg_tbuf, "data length %i must be positive", argi[0]);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "screenpath") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0) {

						failed = 0;
						strcpy(rd->screenpath, tbuf);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "windowsize") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[1], tbuf) != NULL) ;
					else break;

					if (argi[0] >= GP_MIN_SIZE_X && argi[1] >= GP_MIN_SIZE_Y) {

						failed = 0;
						rd->window_size_x = argi[0];
						rd->window_size_y = argi[1];
					}
					else {
						sprintf(msg_tbuf, "too small window sizes %i %i", argi[0], argi[1]);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "language") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					if (argi[0] >= LANG_EN && argi[0] < LANG_END_OF_LIST) {

						failed = 0;
						rd->language = argi[0];
					}
					else {
						sprintf(msg_tbuf, "invalid language number %i", argi[0]);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "colorscheme") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					if (argi[0] >= 0 && argi[0] <= 2) {

						failed = 0;
						rd->colorscheme = argi[0];
					}
					else {
						sprintf(msg_tbuf, "invalid colorscheme number %i", argi[0]);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "antialiasing") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					if (argi[0] >= 0 && argi[0] < 3) {

						failed = 0;
						rd->antialiasing = argi[0];
					}
					else {
						sprintf(msg_tbuf, "invalid antialiasing %i", argi[0]);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "solidfont") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					if (argi[0] >= 0 && argi[0] <= 1) {

						failed = 0;
						rd->solidfont = argi[0];
					}
					else {
						sprintf(msg_tbuf, "invalid solidfont %i", argi[0]);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "thickness") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					if (argi[0] >= 0 && argi[0] < 3) {

						failed = 0;
						rd->thickness = argi[0];
					}
					else {
						sprintf(msg_tbuf, "invalid thickness %i", argi[0]);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "timecol") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					if (argi[0] >= -1 && argi[0] < READ_COLUMN_MAX) {

						failed = 0;
						rd->timecol = argi[0];
					}
					else {
						sprintf(msg_tbuf, "invalid column number %i", argi[0]);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "shortfilename") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					if (argi[0] >= 0) {

						failed = 0;
						rd->shortfilename = argi[0];
					}
					else {
						sprintf(msg_tbuf, "invalid number of dirs %i", argi[0]);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "precision") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					if (argi[0] >= 1 && argi[0] <= 16) {

						failed = 0;
						rd->pl->fprecision = argi[0];
					}
					else {
						sprintf(msg_tbuf, "invalid precision %i", argi[0]);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "delim") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0) {

						failed = 0;
						rd->mk_text.delim = tbuf[0];
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "space") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0) {

						failed = 0;
						strcpy(rd->mk_text.space, rd->mk_config.space);
						strcat(rd->mk_text.space, tbuf);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "lz4_compress") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					if (rd->bind_N != -1) {

						sprintf(msg_tbuf, "unable if dataset was already opened");
						break;
					}

					if (argi[0] >= 0 && argi[0] < 2) {

						failed = 0;
						rd->pl->lz4_compress = argi[0];
					}
					else {
						sprintf(msg_tbuf, "invalid lz4_compress %i", argi[0]);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "load") == 0 || strcmp(tbuf, "follow") == 0) {

				failed = 1;

				flag_follow = (strcmp(tbuf, "follow") == 0) ? 1 : 0;
				flag_stub = 0;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					if (argi[0] < 0 || argi[0] >= PLOT_DATASET_MAX) {

						sprintf(msg_tbuf, "dataset number %i is out of range", argi[0]);
						break;
					}

					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[1], tbuf) != NULL) ;
					else break;

					r = configLexerFSM(rd, pa);

					if (r == 0) {

						if (strcmp(tbuf, "text") == 0) {

							argi[2] = FORMAT_PLAIN_TEXT;
						}
						else if (strcmp(tbuf, "float") == 0) {

							argi[2] = FORMAT_BINARY_FLOAT;
						}
						else if (strcmp(tbuf, "double") == 0) {

							argi[2] = FORMAT_BINARY_DOUBLE;
						}
						else {
							sprintf(msg_tbuf, "invalid file format \"%.80s\"", tbuf);
							break;
						}
					}
					else break;

					if (		argi[2] == FORMAT_BINARY_FLOAT
							|| argi[2] == FORMAT_BINARY_DOUBLE) {

						r = configLexerFSM(rd, pa);

						if (r == 0 && stoi(&rd->mk_config, &argi[3], tbuf) != NULL) ;
						else break;

						flag_stub = 1;
					}

					r = configLexerFSM(rd, pa);

					if (r == 0) {

						rd->data[argi[0]].follow = flag_follow;

						readOpenUnified(rd, argi[0], argi[3], argi[1], tbuf, argi[2]);

						if (		rd->data[argi[0]].fd == NULL
								&& flag_stub != 0) {

							readOpenStub(rd, argi[0], argi[3], argi[1], tbuf, argi[2]);
						}

						failed = 0;
					}
					else break;
				}
				while (0);
			}
			else if (strcmp(tbuf, "bind") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					if (argi[0] >= 0 && argi[0] < PLOT_DATASET_MAX) {

						if (rd->data[argi[0]].format != FORMAT_NONE) {

							failed = 0;
							rd->bind_N = argi[0];
						}
						else {
							sprintf(msg_tbuf, "no dataset has a number %i", argi[0]);
						}
					}
					else {
						sprintf(msg_tbuf, "dataset number %i is out of range", argi[0]);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "group") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					if (argi[0] < 0 || argi[0] >= PLOT_GROUP_MAX) {

						sprintf(msg_tbuf, "group number %i is out of range", argi[0]);
						break;
					}

					if (rd->bind_N == -1) {

						sprintf(msg_tbuf, "no dataset selected");
						break;
					}

					failed = 0;

					do {
						r = configLexerFSM(rd, pa);

						if (r != 0)
							break;

						if (stoi(&rd->mk_config, &argi[1], tbuf) != NULL) ;
						else {
							failed = 1;
							break;
						}

						if (argi[1] >= -1 && argi[1] < rd->pl->data[rd->bind_N].column_N) {

							plotGroupAdd(rd->pl, rd->bind_N, argi[0], argi[1]);
						}
						else {
							failed = 1;

							sprintf(msg_tbuf, "column number %i is out of range", argi[1]);
							break;
						}
					}
					while (1);
				}
				while (0);
			}
			else if (strcmp(tbuf, "deflabel") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					r = configLexerFSM(rd, pa);

					if (argi[0] >= 0 && argi[0] < PLOT_GROUP_MAX) {

						failed = 0;

						plotGroupLabel(rd->pl, argi[0], tbuf);
					}
					else {
						sprintf(msg_tbuf, "group number %i is out of range", argi[0]);
						break;
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "defunwrap") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					if (argi[0] >= 0 && argi[0] < PLOT_GROUP_MAX) {

						failed = 0;

						plotGroupTimeUnwrap(rd->pl, argi[0], 1);
					}
					else {
						sprintf(msg_tbuf, "group number %i is out of range", argi[0]);
						break;
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "defscale") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					r = configLexerFSM(rd, pa);

					if (r == 0 && stod(&rd->mk_config, argd + 0, tbuf) != NULL) ;
					else break;

					r = configLexerFSM(rd, pa);

					if (r == 0 && stod(&rd->mk_config, argd + 1, tbuf) != NULL) ;
					else break;

					if (argi[0] >= 0 && argi[0] < PLOT_GROUP_MAX) {

						failed = 0;

						plotGroupScale(rd->pl, argi[0], argd[0], argd[1]);
					}
					else {
						sprintf(msg_tbuf, "group number %i is out of range", argi[0]);
						break;
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "page") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0) {

						failed = 0;

						if (rd->page_N == -1) {

							rd->page_N = 1;
						}
						else {
							rd->page_N++;
						}

						rd->figure_N = -1;
						rd->page[rd->page_N].busy = 1;

						strcpy(rd->page[rd->page_N].title, tbuf);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "mkpages") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					if (rd->bind_N == -1) {

						sprintf(msg_tbuf, "no dataset selected");
						break;
					}

					if (argi[0] >= -2 && argi[0] < rd->pl->data[rd->bind_N].column_N) {

						failed = 0;
						readMakePages(rd, rd->bind_N, argi[0], 0);
					}
					else {
						sprintf(msg_tbuf, "column number %i is out of range", argi[0]);
						break;
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "label") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					r = configLexerFSM(rd, pa);

					if (rd->page_N == -1) {

						sprintf(msg_tbuf, "no page selected");
						break;
					}

					if (r == 0) {

						if (argi[0] >= 0 && argi[0] < PLOT_AXES_MAX) {

							failed = 0;
							strcpy(rd->page[rd->page_N].ax[argi[0]].label, tbuf);
						}
						else {
							sprintf(msg_tbuf, "axis number %i is out of range", argi[0]);
						}
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "slave") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[1], tbuf) != NULL) ;
					else break;

					r = configLexerFSM(rd, pa);

					if (r == 0 && stod(&rd->mk_config, argd + 0, tbuf) != NULL) ;
					else break;

					r = configLexerFSM(rd, pa);

					if (r == 0 && stod(&rd->mk_config, argd + 1, tbuf) != NULL) ;
					else break;

					if (rd->page_N == -1) {

						sprintf(msg_tbuf, "no page selected");
						break;
					}

					if (argi[0] >= 0 && argi[0] < PLOT_AXES_MAX
						&& argi[1] >= 0 && argi[1] < PLOT_AXES_MAX) {

						failed = 0;
						rd->page[rd->page_N].ax[argi[0]].slave = 1;
						rd->page[rd->page_N].ax[argi[0]].slave_N = argi[1];
						rd->page[rd->page_N].ax[argi[0]].scale = argd[0];
						rd->page[rd->page_N].ax[argi[0]].offset = argd[1];
					}
					else {
						sprintf(msg_tbuf, "axes numbers %i %i are out of range", argi[0], argi[1]);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "figure") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[1], tbuf) != NULL) ;
					else break;

					r = configLexerFSM(rd, pa);

					if (rd->page_N == -1) {

						sprintf(msg_tbuf, "no page selected");
						break;
					}

					if (rd->bind_N == -1) {

						sprintf(msg_tbuf, "no dataset selected");
						break;
					}

					if (argi[0] < -1 || argi[0] >= rd->pl->data[rd->bind_N].column_N
						|| argi[1] < -1 || argi[1] >= rd->pl->data[rd->bind_N].column_N) {

						sprintf(msg_tbuf, "column numbers %i %i are out of range", argi[0], argi[1]);
						break;
					}

					if (r == 0) {

						if (rd->figure_N < PLOT_FIGURE_MAX - 1) {

							failed = 0;
							rd->figure_N++;

							rd->page[rd->page_N].fig[rd->figure_N].busy = 1;
							rd->page[rd->page_N].fig[rd->figure_N].drawing = -1;
							rd->page[rd->page_N].fig[rd->figure_N].dN = rd->bind_N;
							rd->page[rd->page_N].fig[rd->figure_N].cX = argi[0];
							rd->page[rd->page_N].fig[rd->figure_N].cY = argi[1];
							rd->page[rd->page_N].fig[rd->figure_N].aX = 0;
							rd->page[rd->page_N].fig[rd->figure_N].aY = 1;

							strcpy(rd->page[rd->page_N].fig[rd->figure_N].label, tbuf);
						}
						else {
							sprintf(msg_tbuf, "too many figures on page %i", rd->page_N);
						}
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "map") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[1], tbuf) != NULL) ;
					else break;

					if (rd->figure_N == -1) {

						sprintf(msg_tbuf, "no figure selected");
						break;
					}

					if (argi[0] >= 0 && argi[0] < PLOT_AXES_MAX
						&& argi[1] >= 0 && argi[1] < PLOT_AXES_MAX) {

						failed = 0;
						rd->page[rd->page_N].fig[rd->figure_N].aX = argi[0];
						rd->page[rd->page_N].fig[rd->figure_N].aY = argi[1];
					}
					else {
						sprintf(msg_tbuf, "axes numbers %i %i are out of range", argi[0], argi[1]);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "scale") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[0], tbuf) != NULL) ;
					else break;

					r = configLexerFSM(rd, pa);

					if (r == 0 && stod(&rd->mk_config, &argd[0], tbuf) != NULL) ;
					else break;

					r = configLexerFSM(rd, pa);

					if (r == 0 && stod(&rd->mk_config, &argd[1], tbuf) != NULL) ;
					else break;

					if (rd->figure_N == -1) {

						sprintf(msg_tbuf, "no figure selected");
						break;
					}

					if (argi[0] == 0 || argi[0] == 1) {

						failed = 0;
						rd->page[rd->page_N].fig[rd->figure_N].ops[argi[0]].busy = SUBTRACT_SCALE;
						rd->page[rd->page_N].fig[rd->figure_N].ops[argi[0]].scale = argd[0];
						rd->page[rd->page_N].fig[rd->figure_N].ops[argi[0]].offset = argd[1];
					}
					else {
						sprintf(msg_tbuf, "axis number %i is out of range", argi[0]);
					}
				}
				while (0);
			}
			else if (strcmp(tbuf, "drawing") == 0) {

				failed = 1;

				do {
					r = configLexerFSM(rd, pa);

					if (r == 0) {

						if (strcmp(tbuf, "line") == 0)
							argi[0] = FIGURE_DRAWING_LINE;
						else if (strcmp(tbuf, "dash") == 0)
							argi[0] = FIGURE_DRAWING_DASH;
						else if (strcmp(tbuf, "dot") == 0)
							argi[0] = FIGURE_DRAWING_DOT;
						else {
							sprintf(msg_tbuf, "invalid drawing \"%.80s\"", tbuf);
							break;
						}
					}
					else break;

					r = configLexerFSM(rd, pa);

					if (r == 0 && stoi(&rd->mk_config, &argi[1], tbuf) != NULL) ;
					else break;

					if (argi[1] >= 0 && argi[1] <= 16) {

						failed = 0;

						if (rd->figure_N == -1) {

							rd->pl->default_drawing = argi[0];
							rd->pl->default_width = argi[1];
						}
						else {
							rd->page[rd->page_N].fig[rd->figure_N].drawing = argi[0];
							rd->page[rd->page_N].fig[rd->figure_N].width = argi[1];
						}
					}
					else {
						sprintf(msg_tbuf, "figure width %i is out of range", argi[1]);
					}
				}
				while (0);
			}
			else {
				failed = 1;

				sprintf(msg_tbuf, "unknown tokens \"%.80s\"", tbuf);
			}

			if (failed) {

				ERROR("%s:%i: %s\n", pa->file, pa->line_N, msg_tbuf);
			}
		}
		else if (r == 0 && pa->newline == 0) {

			sprintf(msg_tbuf, "extra tokens \"%.80s\"", tbuf);

			ERROR("%s:%i: %s\n", pa->file, pa->line_N, msg_tbuf);
		}
	}
	while (1);
}

void readConfigGP(read_t *rd, const char *file)
{
	FILE		*fd;
	parse_t		pa;

	fd = unified_fopen(file, "r");

	if (fd == NULL) {

		ERROR("fopen(\"%s\"): %s\n", file, strerror(errno));
	}
	else {
		strcpy(pa.file, file);

		pa.fd = fd;
		pa.unchar = -1;
		pa.line_N = 1;
		pa.newline = 1;

		configParseFSM(rd, &pa);

		fclose(fd);
	}
}

int readValidate(read_t *rd)
{
	int		invalid = 0;

	if (rd->pl->font == NULL) {

		plotFontDefault(rd->pl, TTF_ID_ROBOTO_MONO_NORMAL, 24, TTF_STYLE_NORMAL);
	}

	if (rd->page_N == -1) {

		ERROR("No pages specified\n");
		invalid = 1;
	}

	if (rd->bind_N == -1) {

		ERROR("No datasets specified\n");
		invalid = 1;
	}

	rd->page_N = -1;

	return invalid;
}

static void
ansiShort(char *tbuf, const char *text, int allowed)
{
	int		length;

	length = strlen(text);

	if (length > (allowed - 1)) {

		text = utf8_skip_b(text, length - (allowed - 2));

		strcpy(tbuf, "~");
		strcat(tbuf, text);
	}
	else {
		strcpy(tbuf, text);
	}
}

static void
filenameShort(read_t *rd, char *tbuf, const char *file, int allowed)
{
	const char	*eol;
	int		length, ndir;

	file = (file[0] == '.' && file[1] == '/')
		? file + 2 : file;

	if (allowed < 25 || rd->shortfilename != 0) {

		eol = file + strlen(file);
		ndir = rd->shortfilename;

		do {
			if (*eol == '/' || *eol == '\\') {

				if (ndir > 1) ndir--;
				else break;
			}

			if (file == eol)
				break;

			eol--;
		}
		while (1);
	}
	else {
		eol = file;
	}

	length = utf8_length(eol);

	if (length > (allowed - 1)) {

		eol = utf8_skip(eol, length - (allowed - 1));

		strcpy(tbuf, "~");
		strcat(tbuf, eol);
	}
	else {
		if (file == eol) {

			strcpy(tbuf, eol);
		}
		else {
			strcpy(tbuf, "~");
			strcat(tbuf, eol);
		}
	}
}

static void
labelGetUnit(char *tbuf, const char *label, int allowed)
{
	const char		*text;
	int			length;

	text = strchr(label, '@');

	if (text != NULL) {

		text += 1;
		length = utf8_length(text);

		if (length > (allowed - 1)) {

			text = utf8_skip(text, length - (allowed - 1));

			strcpy(tbuf, "~");
			strcat(tbuf, text);
		}
		else {
			strcpy(tbuf, text);
		}
	}
	else {
		*tbuf = 0;
	}
}

void readMakePages(read_t *rd, int dN, int cX, int fromUI)
{
	char		tbuf[READ_FILE_PATH_MAX];
	char		sbuf[READ_FILE_PATH_MAX];
	int		N, pN;

	if (dN < 0 || dN >= PLOT_DATASET_MAX) {

		ERROR("Dataset number is out of range\n");
		return ;
	}

	cX = (cX < -1) ? rd->timecol : cX;
	pN = rd->page_N;

	for (N = 0; N < rd->data[dN].column_N; ++N) {

		if (pN == -1) {

			pN = 1;
		}

		while (rd->page[pN].busy != 0) {

			pN++;

			if (pN >= READ_PAGE_MAX)
				break;
		}

		if (pN >= READ_PAGE_MAX)
			break;

		rd->page[pN].busy = 2;

		filenameShort(rd, tbuf, rd->data[dN].file, 200);

		sprintf(sbuf, "%s: [%2i] %.75s", tbuf, N, rd->data[dN].label[N]);
		ansiShort(rd->page[pN].title, sbuf, PLOT_STRING_MAX);

		rd->page[pN].fig[0].busy = 1;
		rd->page[pN].fig[0].drawing = -1;
		rd->page[pN].fig[0].dN = dN;
		rd->page[pN].fig[0].cX = cX;
		rd->page[pN].fig[0].cY = N;
		rd->page[pN].fig[0].aX = 0;
		rd->page[pN].fig[0].aY = 1;

		filenameShort(rd, tbuf, rd->data[dN].file, 20);

		sprintf(sbuf, "%s: [%2i] %.75s", tbuf, N, rd->data[dN].label[N]);
		ansiShort(rd->page[pN].fig[0].label, sbuf, PLOT_STRING_MAX);

		labelGetUnit(tbuf, rd->data[dN].label[N], 20);
		strcpy(rd->page[pN].ax[1].label, tbuf);
	}

	if (fromUI == 0) {

		rd->page_N = pN;
		rd->figure_N = -1;
	}
}

void readDatasetClean(read_t *rd, int dN)
{
	page_t		*pg;
	int		N, pN, pW, fN;

	if (dN < 0 || dN >= PLOT_DATASET_MAX) {

		ERROR("Dataset number is out of range\n");
		return ;
	}

	pN = 1;

	do {
		if (rd->page[pN].busy != 0) {

			pg = rd->page + pN;

			for (fN = 0; fN < PLOT_FIGURE_MAX; ++fN) {

				if (pg->fig[fN].dN == dN)
					pg->fig[fN].busy = 0;
			}

			N = 0;

			for (fN = 0; fN < PLOT_FIGURE_MAX; ++fN) {

				if (pg->fig[fN].busy != 0)
					++N;
			}

			if (N == 0) {

				memset(&rd->page[pN], 0, sizeof(rd->page[0]));
			}
		}

		pN += 1;

		if (pN >= READ_PAGE_MAX)
			break;
	}
	while (1);

	pN = 1;
	pW = 1;

	do {
		if (rd->page[pN].busy != 0) {

			if (pN != pW) {

				memcpy(&rd->page[pW], &rd->page[pN], sizeof(rd->page[0]));
				memset(&rd->page[pN], 0, sizeof(rd->page[0]));
			}

			pW += 1;
		}

		pN += 1;

		if (pN >= READ_PAGE_MAX)
			break;
	}
	while (1);

	if (rd->data[dN].fd != NULL) {

		readClose(rd, dN);
	}

	memset(&rd->data[dN], 0, sizeof(rd->data[0]));

	plotFigureGarbage(rd->pl, dN);

	plotDataRangeCacheClean(rd->pl, dN);
	plotDataClean(rd->pl, dN);
}

int readGetTimeColumn(read_t *rd, int dN)
{
	page_t		*pg;
	int		cNP, pN;

	cNP = -2;
	pN = 1;

	if (dN < 0 || dN >= PLOT_DATASET_MAX) {

		ERROR("Dataset number is out of range\n");
		return cNP;
	}

	do {
		if (rd->page[pN].busy == 2) {

			pg = rd->page + pN;

			if (pg->fig[0].dN == dN) {

				cNP = pg->fig[0].cX;
				break;
			}
		}

		pN += 1;

		if (pN >= READ_PAGE_MAX)
			break;
	}
	while (1);

	return cNP;
}

void readSetTimeColumn(read_t *rd, int dN, int cX)
{
	page_t		*pg;
	int		gN, cNP, pN;

	if (dN < 0 || dN >= PLOT_DATASET_MAX) {

		ERROR("Dataset number is out of range\n");
		return ;
	}

	if (cX < -1 || cX >= rd->pl->data[dN].column_N + PLOT_SUBTRACT) {

		ERROR("Time column number %i is out of range\n", cX);
		return ;
	}

	cNP = -2;
	pN = 1;

	do {
		if (rd->page[pN].busy == 2) {

			pg = rd->page + pN;

			if (pg->fig[0].dN == dN) {

				cNP = (cNP == -2) ? pg->fig[0].cX : cNP;

				pg->fig[0].cX = cX;
			}
		}

		pN += 1;

		if (pN >= READ_PAGE_MAX)
			break;
	}
	while (1);

	if (cNP != -2) {

		gN = rd->pl->data[dN].map[cNP];

		if (gN != -1) {

			rd->pl->data[dN].map[cNP] = -1;
			rd->pl->data[dN].map[cX] = gN;
		}
	}
}

static int
timeDataMap(plot_t *pl, int dN, int cN)
{
	int		gN, cMAP;

	if (dN < 0 || dN >= PLOT_DATASET_MAX) {

		ERROR("Dataset number is out of range\n");
		return cN;
	}

	if (cN < -1 || cN >= pl->data[dN].column_N + PLOT_SUBTRACT) {

		ERROR("Column number %i is out of range\n", cN);
		return cN;
	}

	if (pl->data[dN].map == NULL) {

		ERROR("Dataset number %i was not allocated\n", dN);
		return cN;
	}

	gN = pl->data[dN].map[cN];

	if (gN != -1) {

		if (pl->group[gN].op_time_unwrap != 0) {

			cMAP = plotGetSubtractTimeUnwrap(pl, dN, cN);

			if (cMAP != -1) {

				pl->data[dN].map[cMAP] = gN;
				cN = cMAP;
			}
		}
	}

	return cN;
}

static int
scaleDataMap(plot_t *pl, int dN, int cN, fig_ops_t *ops)
{
	int		gN, cMAP;

	if (dN < 0 || dN >= PLOT_DATASET_MAX) {

		ERROR("Dataset number is out of range\n");
		return cN;
	}

	if (cN < -1 || cN >= pl->data[dN].column_N + PLOT_SUBTRACT) {

		ERROR("Column number %i is out of range\n", cN);
		return cN;
	}

	if (pl->data[dN].map == NULL) {

		ERROR("Dataset number %i was not allocated\n", dN);
		return cN;
	}

	gN = pl->data[dN].map[cN];

	if (gN != -1) {

		if (pl->group[gN].op_scale != 0) {

			cMAP = plotGetSubtractScale(pl, dN, cN,
					pl->group[gN].scale,
					pl->group[gN].offset);

			if (cMAP != -1) {

				pl->data[dN].map[cMAP] = gN;
				cN = cMAP;
			}
		}
	}

	if (ops->busy == SUBTRACT_SCALE) {

		cMAP = plotGetSubtractScale(pl, dN, cN,
				ops->scale, ops->offset);

		if (cMAP != -1) {

			cN = cMAP;
		}
	}
	else if (ops->busy == SUBTRACT_BINARY_SUBTRACTION) {

		cMAP = plotGetSubtractBinary(pl, dN, ops->busy,
				cN, ops->column_2);

		if (cMAP != -1) {

			cN = cMAP;

			cMAP = plotGetSubtractScale(pl, dN, cN,
					ops->scale, ops->offset);

			if (cMAP != -1) {

				cN = cMAP;
			}
		}
	}

	return cN;
}

void readSelectPage(read_t *rd, int pN)
{
	plot_t		*pl = rd->pl;
	page_t		*pg;
	int		N, cX, cY;

	if (pN < 0 || pN >= READ_PAGE_MAX) {

		ERROR("Page number is out of range\n");
		return ;
	}

	pg = rd->page + pN;

	if (pg->busy == 0)
		return;

	rd->page_N = pN;

	plotFigureClean(pl);

	plotDataRangeCacheSubtractClean(pl);
	plotDataSubtractClean(pl);

	for (N = 0; N < PLOT_FIGURE_MAX; ++N) {

		if (pg->fig[N].busy != 0) {

			cX = timeDataMap(pl, pg->fig[N].dN, pg->fig[N].cX);
			cX = scaleDataMap(pl, pg->fig[N].dN, cX, &pg->fig[N].ops[0]);
			cY = scaleDataMap(pl, pg->fig[N].dN, pg->fig[N].cY, &pg->fig[N].ops[1]);

			plotFigureAdd(pl, N, pg->fig[N].dN, cX, cY, pg->fig[N].aX,
					pg->fig[N].aY, pg->fig[N].label);

			if (pg->fig[N].drawing != -1) {

				pl->figure[N].drawing = pg->fig[N].drawing;
				pl->figure[N].width = pg->fig[N].width;
			}
		}
	}

	for (N = 0; N < PLOT_AXES_MAX; ++N) {

		plotAxisLabel(pl, N, pg->ax[N].label);

		if (pg->ax[N].slave != 0) {

			plotAxisSlave(pl, N, pg->ax[N].slave_N, pg->ax[N].scale,
					pg->ax[N].offset, AXIS_SLAVE_ENABLE);
		}
	}

	plotLayout(pl);
	plotAxisScaleDefault(pl);
}

static int
combineGetFreeAxis(plot_t *pl, int *map)
{
	int		N, J, sF, aN = -1;

	for (N = 0; N < PLOT_AXES_MAX; ++N) {

		if (pl->axis[N].busy == AXIS_FREE) {

			sF = 0;

			for (J = 0; J < PLOT_AXES_MAX; ++J) {

				if (map[J] == N) {

					sF = 1;
					break;
				}
			}

			if (sF == 0) {

				aN = N;
				break;
			}
		}
	}

	return aN;
}

static int
combineGetMappedAxis(plot_t *pl, int dN, int cN)
{
	int		N, aN = -1;
	int		gN, *map;

	if (dN < 0 || dN >= PLOT_DATASET_MAX) {

		ERROR("Dataset number is out of range\n");
		return -1;
	}

	if (cN < -1 || cN >= pl->data[dN].column_N + PLOT_SUBTRACT) {

		ERROR("Column number %i is out of range\n", cN);
		return -1;
	}

	for (N = 0; N < PLOT_FIGURE_MAX; ++N) {

		if (pl->figure[N].busy != 0) {

			if (pl->figure[N].data_N == dN) {

				if (pl->figure[N].column_X == cN) {

					aN = pl->figure[N].axis_X;
					break;
				}
				else if (pl->figure[N].column_Y == cN) {

					aN = pl->figure[N].axis_Y;
					break;
				}
			}

			gN = pl->data[dN].map[cN];

			if (gN != -1) {

				map = pl->data[pl->figure[N].data_N].map;

				if (gN == map[pl->figure[N].column_X]) {

					aN = pl->figure[N].axis_X;
					break;
				}
				else if (gN == map[pl->figure[N].column_Y]) {

					aN = pl->figure[N].axis_Y;
					break;
				}
			}
		}
	}

	return aN;
}

void readCombinePage(read_t *rd, int pN, int remap)
{
	plot_t		*pl = rd->pl;
	page_t		*pg;
	int		N, fN, bN, cX, cY, aX, aY;
	int		map[PLOT_AXES_MAX];

	if (pN < 0 || pN >= READ_PAGE_MAX) {

		ERROR("Page number is out of range\n");
		return ;
	}

	if (pN == rd->page_N)
		return;

	pg = rd->page + pN;

	if (pg->busy == 0)
		return;

	for (N = 0; N < PLOT_AXES_MAX; ++N)
		map[N] = -1;

	if (remap == 0) {

		map[0] = pl->on_X;
		map[1] = pl->on_Y;
	}
	else {
		for (N = 0; N < PLOT_FIGURE_MAX; ++N) {

			if (pg->fig[N].busy != 0) {

				if (map[pg->fig[N].aX] == -1) {

					aX = combineGetMappedAxis(pl, pg->fig[N].dN, pg->fig[N].cX);
					aX = (aX == -1) ? combineGetFreeAxis(pl, map) : aX;
					map[pg->fig[N].aX] = aX;
				}

				if (map[pg->fig[N].aY] == -1) {

					aY = combineGetMappedAxis(pl, pg->fig[N].dN, pg->fig[N].cY);
					aY = (aY == -1) ? combineGetFreeAxis(pl, map) : aY;
					map[pg->fig[N].aY] = aY;
				}
			}
		}
	}

	for (N = 0; N < PLOT_FIGURE_MAX; ++N) {

		if (pg->fig[N].busy != 0) {

			fN = plotGetFreeFigure(pl);

			if (fN == -1) {

				ERROR("No free figure to combine\n");
				break;
			}

			aX = (map[pg->fig[N].aX] != -1) ? map[pg->fig[N].aX] : pg->fig[N].aX;
			aY = (map[pg->fig[N].aY] != -1) ? map[pg->fig[N].aY] : pg->fig[N].aY;

			cX = timeDataMap(pl, pg->fig[N].dN, pg->fig[N].cX);
			cX = scaleDataMap(pl, pg->fig[N].dN, cX, &pg->fig[N].ops[0]);
			cY = scaleDataMap(pl, pg->fig[N].dN, pg->fig[N].cY, &pg->fig[N].ops[1]);

			plotFigureAdd(pl, fN, pg->fig[N].dN, cX, cY, aX, aY, "");

			sprintf(pl->figure[fN].label, "%i: %.75s", pN, pg->fig[N].label);

			if (pg->fig[N].drawing != -1) {

				pl->figure[fN].drawing = pg->fig[N].drawing;
				pl->figure[fN].width = pg->fig[N].width;
			}
		}
	}

	for (N = 0; N < PLOT_AXES_MAX; ++N) {

		aX = (map[N] != -1) ? map[N] : N;

		if (pl->axis[aX].label[0] == 0) {

			plotAxisLabel(pl, aX, pg->ax[N].label);
		}

		if (pg->ax[N].slave != 0) {

			aY = pg->ax[N].slave_N;
			bN = (map[aY] != -1) ? map[aY] : aY;

			plotAxisSlave(pl, aX, bN, pg->ax[N].scale,
					pg->ax[N].offset, AXIS_SLAVE_ENABLE);
		}
	}

	plotLayout(pl);
	plotAxisScaleDefault(pl);
}

void readDataReload(read_t *rd)
{
	int		dN;

	for (dN = 0; dN < PLOT_DATASET_MAX; ++dN) {

		if (		rd->data[dN].format != FORMAT_NONE
				&& rd->data[dN].file[0] != 0) {

			readOpenUnified(rd, dN, rd->data[dN].column_N,
					rd->data[dN].length_N, rd->data[dN].file,
					rd->data[dN].format);
		}
	}
}

