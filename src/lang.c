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

#include "lang.h"

void langFill(lang_t *la, int lang)
{
	if (lang == LANG_EN) {

		la->global_menu =

			"    Zoom ...\0"
			"P   Take a screenshot\0"
			"F   Fullscreen mode toggle\0"
			"    Appearance ...\0"
			"    Data management ...\0"
			"En  Go to the page ...\0"
			"C   Combine with page ...\0"
			"B   No remap combine ...\0"
			"R   Create subtraction\0"
			"M   Place figure markers\0"
			"T   Data slice (on X)\0"
			"K   Compact axes mode\0"
			"E   Exponential mode\0"
			"Y   Yank to clipboard\0"
			"    Language ...\0"
			"    Page title ...\0"
			"    Quit\0"

			"\0";

		la->global_zoom_menu =

			"A   Auto fit to page\0"
			"Q   Equal scales\0"
			"G   Grid align\0"
			"W   Staked (on Y)\0"

			"\0";

		la->global_appearance_menu =

			"L   Change a colorscheme  [ %s ]     \0"
			"    Change a font         [ %s ]\0"
			"    Drawing antialiasing  [ %s ]\0"
			"    Line thickness        [ %s ]\0"
			"    Hinting mode          [ %s ]\0"
			"    Font size             [ %s ]\0"

			"\0";

		la->global_data_menu =

			"U   Reload all data files\0"
			"O   Open data file ...\0"
			"    Dataset customization ...\0"
			"    Edit configuration ...\0"
			"    Default configuration\0"

			"\0";

		la->dataset_menu[0] = " Time column  [%3i]";
		la->dataset_menu[1] = " Time unwrap  [ %s ]";
		la->dataset_menu[2] = " Time scale   [%s]";
		la->dataset_menu[3] = " Length       [%3i]  %iM";

		la->axis_menu =

			"    Zoom ...\0"
			"S   Slave mode\0"
			"X   Remove the axis\0"
			"T   Data slice\0"
			"K   Compact mode      [ %s ]\0"
			"E   Exponential mode  [ %s ]\0"
			"    Label ...\0"

			"\0";

		la->axis_zoom_menu =

			"A   Auto fit to page\0"
			"    Selective auto fit\0"
			"    Copy scale from active\0"
			"    Edit ...\0"

			"\0";

		la->figure_menu =

			"    Move to active axes\0"
			"    Create individual axes\0"
			"X   Remove the figure\0"
			"    Subtract operation ...\0"
			"    Edit the figure ...\0"

			"\0";

		la->figure_edit_menu =

			"    Drawing ...\0"
			"    Thickness ...\0"
			"    Color ...\0"
			"    Data X ...\0"
			"    Data Y ...\0"
			"    Label ...\0"

			"\0";

		la->figure_edit_drawing_menu =

			"    Line  1p\0"
			"    Line  2p\0"
			"    Line  4p\0"
			"    Line  6p\0"
			"    Dash  1p\0"
			"    Dash  2p\0"
			"    Dash  4p\0"
			"    Dash  6p\0"
			"    Dot   1p\0"
			"    Dot   2p\0"
			"    Dot   4p\0"
			"    Dot   6p\0"

			"\0";

		la->figure_operation_menu = 

			"    Duplicate figure\0"
			"    Time unwrap\0"
			"    Scale (on X) ...\0"
			"    Scale (on Y) ...\0"
			"R   Add B subtraction\0"
			"    Add B addition\0"
			"    Add B multiplication\0"
			"    Add B hypotenuse\0"
			"    Add F sequential difference\0"
			"    Add F cumulative sum\0"
			"    Add F bit field ...\0"
			"    Add F low pass ...\0"

			"\0";

		la->legend_menu =

			"    Drawing primitive ...\0"
			"    Transparency mode   [ %s ]\0"

			"\0";

		la->cancel_menu =

			"    Cancel\0"
			"    OK\0"

			"\0";

		la->page_title_edit = "Page Title";
		la->figure_label_edit = "Figure Label";
		la->axis_label_edit = "Axis Label";
		la->scale_offset_edit = "Scale and offset";
		la->file_name_edit = "File Name";
		la->bit_number_edit = "Bit Range";
		la->low_pass_edit = "Low Pass Gain";
		la->length_edit = "Length";
		la->figure_thickness_edit = "Thickness";
		la->font_size_edit = "Font size";
	}
	else if (lang == LANG_RU) {

		la->global_menu =

			"    Масштаб ...\0"
			"P   Сделать снимок\0"
			"F   Режим полного экрана\0"
			"    Внешний вид ...\0"
			"    Управление данными ...\0"
			"En  Перейти к странице ...\0"
			"C   Комбинировать страницы ...\0"
			"B   Без переназначения ...\0"
			"R   Создать вычитание\0"
			"M   Разместить маркеры фигур\0"
			"T   Нарезка данных (по X)\0"
			"K   Режим компактных осей\0"
			"E   Экспонентный режим\0"
			"Y   Копировать в буфер\0"
			"    Язык ...\0"
			"    Текст заголовка ...\0"
			"    Выход\0"

			"\0";

		la->global_zoom_menu =

			"A   Автоматически на всю страницу\0"
			"Q   Одинаковый масштаб по осям\0"
			"G   Выравнивание сетки\0"
			"W   Укладка (по Y)\0"

			"\0";

		la->global_appearance_menu =

			"L   Сменить цветовую схему  [ %s ]     \0"
			"    Переключение шрифтов    [ %s ]\0"
			"    Сглаживание рисования   [ %s ]\0"
			"    Толщина линий           [ %s ]\0"
			"    Хинтинг шрифта          [ %s ]\0"
			"    Размер шрифта           [ %s ]\0"

			"\0";

		la->global_data_menu =

			"U   Перезагрузить все данные\0"
			"O   Открыть файл данных ...\0"
			"    Настройка наборов данных ...\0"
			"    Редактировать конфигурацию ...\0"
			"    Сбросить конфигурацию\0"

			"\0";

		la->dataset_menu[0] = " Столбец времени   [%3i]";
		la->dataset_menu[1] = " Развернуть время  [ %s ]";
		la->dataset_menu[2] = " Масштаб времени   [%s]";
		la->dataset_menu[3] = " Длина             [%3i]  %iM";

		la->axis_menu =

			"    Масштаб ...\0"
			"S   Подчиненный режим\0"
			"X   Удалить эту ось\0"
			"T   Нарезка данных\0"
			"K   Компактный режим    [ %s ]\0"
			"E   Экспонентный режим  [ %s ]\0"
			"    Текст ...\0"

			"\0";

		la->axis_zoom_menu =

			"A   Автоматически на всю страницу\0"
			"    Выборочный масштаб\0"
			"    Копировать с активной оси\0"
			"    Редактировать ...\0"

			"\0";

		la->figure_menu =

			"    Переместить на активные оси\0"
			"    Создать индивидуальные оси\0"
			"X   Удалить фигуру\0"
			"    Операции вычитания ...\0"
			"    Редактировать фигуру ...\0"

			"\0";

		la->figure_edit_menu =

			"    Рисование ...\0"
			"    Толщина ...\0"
			"    Цвет ...\0"
			"    Данные X ...\0"
			"    Данные Y ...\0"
			"    Текст ...\0"

			"\0";

		la->figure_edit_drawing_menu =

			"    Линия  1п\0"
			"    Линия  2п\0"
			"    Линия  4п\0"
			"    Линия  6п\0"
			"    Черта  1п\0"
			"    Черта  2п\0"
			"    Черта  4п\0"
			"    Черта  6п\0"
			"    Точка  1п\0"
			"    Точка  2п\0"
			"    Точка  4п\0"
			"    Точка  6п\0"

			"\0";

		la->figure_operation_menu =

			"    Дублировать фигуру\0"
			"    Развернуть время\0"
			"    Масштаб (по X) ...\0"
			"    Масштаб (по Y) ...\0"
			"R   Добавить B вычитание\0"
			"    Добавить B сложение\0"
			"    Добавить B умножение\0"
			"    Добавить B гипотенузу\0"
			"    Добавить F последовательные разности\0"
			"    Добавить F накопленные суммы\0"
			"    Добавить F битовое поле ...\0"
			"    Добавить F фильтр НЧ ...\0"

			"\0";

		la->legend_menu =

			"    Примитив рисования ...\0"
			"    Режим прозрачности   [ %s ]\0"

			"\0";

		la->cancel_menu =

			"    Отмена\0"
			"    OK\0"

			"\0";

		la->page_title_edit = "Текст Заголовка";
		la->figure_label_edit = "Текст Фигуры";
		la->axis_label_edit = "Текст Оси";
		la->scale_offset_edit = "Масштаб и смещение";
		la->file_name_edit = "Имя Файла";
		la->bit_number_edit = "Дипазон Разрядов";
		la->low_pass_edit = "Коэффициент НЧ фильтра";
		la->length_edit = "Длина";
		la->figure_thickness_edit = "Толщина";
		la->font_size_edit = "Размер шрифта";
	}

	la->figure_edit_color_menu =

		"          \0"
		"          \0"
		"          \0"
		"          \0"
		"          \0"
		"          \0"
		"          \0"
		"          \0"

		"\0";

	la->global_lang_menu =

		"    English\0"
		"    Russian (Русский)\0"

		"\0";
}
