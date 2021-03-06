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

#ifndef _H_PLOT_
#define _H_PLOT_

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "draw.h"
#include "lse.h"
#include "scheme.h"

#ifdef ERROR
#undef ERROR
#endif

#ifdef FP_NAN
#undef FP_NAN
#endif

#define ERROR(fmt, ...)		fprintf(stderr, "%s:%i: " fmt, __FILE__, __LINE__, ## __VA_ARGS__)
#define FP_NAN			fp_nan()

#define PLOT_DATASET_MAX			10
#define PLOT_CHUNK_SIZE				16777216
#define PLOT_CHUNK_MAX				2000
#define PLOT_CHUNK_CACHE			8
#define PLOT_RCACHE_SIZE			40
#define PLOT_SLICE_SPAN				4
#define PLOT_AXES_MAX				9
#define PLOT_FIGURE_MAX				8
#define PLOT_DATA_BOX_MAX			8
#define PLOT_POLYFIT_MAX			7
#define PLOT_SUBTRACT				10
#define PLOT_GROUP_MAX				40
#define PLOT_MARK_MAX				50
#define PLOT_SKETCH_CHUNK_SIZE			32768
#define PLOT_SKETCH_MAX				800
#define PLOT_STRING_MAX				200

enum {
	TTF_ID_NONE			= 0,
	TTF_ID_ROBOTO_MONO_NORMAL,
	TTF_ID_ROBOTO_MONO_THIN
};

enum {
	AXIS_FREE			= 0,
	AXIS_BUSY_X,
	AXIS_BUSY_Y
};

enum {
	AXIS_SLAVE_DISABLE		= 0,
	AXIS_SLAVE_ENABLE,
	AXIS_SLAVE_HOLD_AS_IS
};

enum {
	FIGURE_DRAWING_LINE		= 0,
	FIGURE_DRAWING_DASH,
	FIGURE_DRAWING_DOT
};

enum {
	SUBTRACT_FREE			= 0,
	SUBTRACT_TIME_UNWRAP,
	SUBTRACT_SCALE,
	SUBTRACT_BINARY_SUBTRACTION,
	SUBTRACT_BINARY_ADDITION,
	SUBTRACT_BINARY_MULTIPLICATION,
	SUBTRACT_BINARY_HYPOTENUSE,
	SUBTRACT_FILTER_DIFFERENCE,
	SUBTRACT_FILTER_CUMULATIVE,
	SUBTRACT_FILTER_BITMASK,
	SUBTRACT_FILTER_LOW_PASS,
	SUBTRACT_RESAMPLE,
	SUBTRACT_POLYFIT
};

enum {
	SKETCH_STARTED			= 0,
	SKETCH_INTERRUPTED,
	SKETCH_FINISHED
};

enum {
	DATA_BOX_FREE			= 0,
	DATA_BOX_SLICE,
	DATA_BOX_POLYFIT
};

typedef double			fval_t;

typedef struct {

	draw_t			*dw;
	scheme_t		*sch;

	struct {

		int		column_N;
		int		length_N;

		int		chunk_SHIFT;
		int		chunk_MASK;

		int		chunk_bSIZE;

		struct {

			fval_t		*raw;

			int		chunk_N;
			int		dirty;
		}
		cache[PLOT_CHUNK_CACHE];

		int		cache_ID;

		struct {

			void		*raw;
			int		length;
		}
		compress[PLOT_CHUNK_MAX];

		fval_t		*raw[PLOT_CHUNK_MAX];
		int		*map;

		int		head_N;
		int		tail_N;
		int		id_N;

		struct {

			int	busy;

			union {

				struct {

					int	column_1;

					double	unwrap;
					double	prev;
					double	prev2;
				}
				time;

				struct {

					int	column_1;

					double	scale;
					double	offset;
				}
				scale;

				struct {

					int	column_1;
					int	column_2;
				}
				binary;

				struct {

					int	column_1;

					double	arg_1;
					double	arg_2;

					double	state;
				}
				filter;

				struct {

					int	column_X;
					int	column_in_X;
					int	column_in_Y;

					int	in_data_N;
				}
				resample;

				struct {

					int	column_X;
					int	column_Y;

					int	poly_N;
					double	coefs[PLOT_POLYFIT_MAX + 1];
				}
				polyfit;
			}
			op;
		}
		sub[PLOT_SUBTRACT];

		int		sub_N;
	}
	data[PLOT_DATASET_MAX];

	struct {

		int		busy;

		int		data_N;
		int		column_N;

		struct {

			int		computed;
			int		finite;

			fval_t		fmin;
			fval_t		fmax;
		}
		chunk[PLOT_CHUNK_MAX];

		int		cached;

		fval_t		fmin;
		fval_t		fmax;
	}
	rcache[PLOT_RCACHE_SIZE];

	struct {

		int		busy;
		int		lock_scale;

		int		slave;
		int		slave_N;

		double		scale;
		double		offset;

		char		label[PLOT_STRING_MAX];

		int		compact;
		int		expen;

		int		_pos;
		double		_tih;
		double		_tis;
	}
	axis[PLOT_AXES_MAX];

	struct {

		int		busy;
		int		hidden;

		int		drawing;
		int		width;

		int		data_N;
		int		column_X;
		int		column_Y;
		int		axis_X;
		int		axis_Y;

		double		mark_X[PLOT_MARK_MAX];
		double		mark_Y[PLOT_MARK_MAX];

		int		slice_busy;
		double		slice_X;
		double		slice_Y;
		double		slice_base_X;
		double		slice_base_Y;

		char		label[PLOT_STRING_MAX];
	}
	figure[PLOT_FIGURE_MAX];

	struct {

		int		op_time_unwrap;
		int		op_scale;

		double		scale;
		double		offset;

		char		label[PLOT_STRING_MAX];
	}
	group[PLOT_GROUP_MAX];

	clipBox_t		viewport;
	clipBox_t		screen;

	TTF_Font		*font;

	lse_t			lsq;

	int			rcache_ID;
	int			rcache_wipe_data_N;
	int			rcache_wipe_chunk_N;

	int			legend_X;
	int			legend_Y;
	int			legend_size_X;
	int			legend_N;

	int			data_box_on;
	int			data_box_X;
	int			data_box_Y;
	int			data_box_size_X;
	int			data_box_N;
	char			data_box_text[PLOT_DATA_BOX_MAX][PLOT_STRING_MAX];

	int			slice_on;
	int			slice_range_on;
	int			slice_axis_N;

	struct {

		int		sketch;

		int		rN;
		int		id_N;

		int		skipped;
		int		line;

		double		last_X;
		double		last_Y;

		int		list_self;
	}
	draw[PLOT_FIGURE_MAX];

	int			draw_in_progress;

	struct {

		int		figure_N;

		int		drawing;
		int		width;

		double		*chunk;
		int		length;

		int		linked;
	}
	sketch[PLOT_SKETCH_MAX];

	int			sketch_list_garbage;
	int			sketch_list_todraw;
	int			sketch_list_current;
	int			sketch_list_current_end;

	int			layout_font_ttf;
	int			layout_font_pt;
	int			layout_font_height;
	int			layout_font_long;
	int			layout_border;
	int			layout_axis_box;
	int			layout_label_box;
	int			layout_tick_tooth;
	int			layout_grid_dash;
	int			layout_grid_space;
	int			layout_drawing_dash;
	int			layout_drawing_space;
	int			layout_mark;
	int			layout_fence_dash;
	int			layout_fence_space;
	int			layout_fence_point;

	int			on_X;
	int			on_Y;

	int			hover_figure;
	int			hover_legend;
	int			hover_data_box;
	int			hover_axis;

	int			mark_on;
	int			mark_N;

	int			default_drawing;
	int			default_width;

	int			transparency_mode;
	int			fprecision;
	int			lz4_compress;

	int			shift_on;
}
plot_t;

double fp_nan();
int fp_isfinite(double x);

plot_t *plotAlloc(draw_t *dw, scheme_t *sch);
void plotClean(plot_t *pl);

void plotFontDefault(plot_t *pl, int ttfnum, int ptsize, int style);
void plotFontOpen(plot_t *pl, const char *file, int ptsize, int style);

unsigned long long plotDataMemoryUsage(plot_t *pl, int dN);
unsigned long long plotDataMemoryUncompressed(plot_t *pl, int dN);
void plotDataAlloc(plot_t *pl, int dN, int cN, int lN);
void plotDataResize(plot_t *pl, int dN, int lN);
int plotDataSpaceLeft(plot_t *pl, int dN);
void plotDataGrowUp(plot_t *pl, int dN);
void plotDataSubtract(plot_t *pl, int dN, int cN);
void plotDataSubtractClean(plot_t *pl);
void plotDataInsert(plot_t *pl, int dN, const fval_t *row);
void plotDataClean(plot_t *pl, int dN);

void plotDataRangeCacheClean(plot_t *pl, int dN);
void plotDataRangeCacheSubtractClean(plot_t *pl);
int plotDataRangeCacheFetch(plot_t *pl, int dN, int cN);

void plotAxisLabel(plot_t *pl, int aN, const char *label);
void plotAxisScaleManual(plot_t *pl, int aN, double min, double max);
void plotAxisScaleAutoCond(plot_t *pl, int aN, int bN);
void plotAxisScaleLock(plot_t *pl, int lock);
void plotAxisScaleAuto(plot_t *pl, int aN);
void plotAxisScaleDefault(plot_t *pl);
void plotAxisScaleZoom(plot_t *pl, int aN, int origin, double zoom);
void plotAxisScaleMove(plot_t *pl, int aN, int move);
void plotAxisScaleEqual(plot_t *pl);
void plotAxisScaleGridAlign(plot_t *pl);
void plotAxisScaleStaked(plot_t *pl);

int plotAxisGetByClick(plot_t *pl, int cur_X, int cur_Y);
double plotAxisConv(plot_t *pl, int aN, double fval);
double plotAxisConvInv(plot_t *pl, int aN, double px);
void plotAxisSlave(plot_t *pl, int aN, int bN, double scale, double offset, int action);
void plotAxisRemove(plot_t *pl, int aN);

void plotFigureAdd(plot_t *pl, int fN, int dN, int nX, int nY, int aX, int aY, const char *label);
void plotFigureRemove(plot_t *pl, int fN);
void plotFigureGarbage(plot_t *pl, int dN);
void plotFigureMoveAxes(plot_t *pl, int fN);
void plotFigureMakeIndividualAxes(plot_t *pl, int fN);
void plotFigureExchange(plot_t *pl, int fN_1, int fN_2);

int plotGetSubtractTimeUnwrap(plot_t *pl, int dN, int cN);
int plotGetSubtractScale(plot_t *pl, int dN, int cN, double scale, double offset);
int plotGetSubtractResample(plot_t *pl, int dN, int cN_X, int in_dN, int in_cN_X, int in_cN_Y);
int plotGetSubtractBinary(plot_t *pl, int dN, int opSUB, int cN_1, int cN_2);
int plotGetFreeFigure(plot_t *pl);

void plotFigureSubtractTimeUnwrap(plot_t *pl, int fN_1);
void plotFigureSubtractScale(plot_t *pl, int fN_1, int aBUSY, double scale, double offset);
void plotFigureSubtractFilter(plot_t *pl, int fN_1, int opSUB, double arg_1, double arg_2);
void plotFigureSubtractSwitch(plot_t *pl, int opSUB);
void plotFigureSubtractPolifit(plot_t *pl, int fN_1, int poly_N);
void plotFigureClean(plot_t *pl);
void plotSketchClean(plot_t *pl);

void plotGroupAdd(plot_t *pl, int dN, int gN, int cN);
void plotGroupLabel(plot_t *pl, int gN, const char *label);
void plotGroupTimeUnwrap(plot_t *pl, int gN, int unwrap);
void plotGroupScale(plot_t *pl, int gN, double scale, double offset);

void plotSliceSwitch(plot_t *pl);
void plotSliceTrack(plot_t *pl, int cur_X, int cur_Y);

int plotLegendGetByClick(plot_t *pl, int cur_X, int cur_Y);
int plotLegendBoxGetByClick(plot_t *pl, int cur_X, int cur_Y);
int plotDataBoxGetByClick(plot_t *pl, int cur_X, int cur_Y);

void plotLayout(plot_t *pl);
void plotDraw(plot_t *pl, SDL_Surface *surface);

#endif /* _H_PLOT_ */

