//  file: simplicity.c
//  desc: Simplicity International
//        Modified from Pebble Simplicity watch source code.
//        Designed to work with SetPebble.com custom settings.
//        The files http.h and http.c are from:  http://github.com/Katharine/http-watch/
//  date: October 20, 2013

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "http.h"

//  app information
//    see http://github.com/Katharine/http-watch/ for why the UUID is set to this value

#define MY_UUID { 0x91, 0x41, 0xB6, 0x28, 0xBC, 0x89, 0x49, 0x8E, 0xB1, 0x47, 0x04, 0x9F, 0x49, 0xC0, 0x99, 0xAD }//  httPebble
PBL_APP_INFO(MY_UUID, "Simplicity Int'l", "SetPebble", 1, 6, RESOURCE_ID_IMAGE_MENU_ICON, APP_INFO_WATCH_FACE);

//  constants and enumerators

const int kAppId = 0x16315378;    //  unique app identifier
const int kCookie = 1949328672;   //  unique cookie
const int kNumLanguages = 10;     //  number of supported languages
enum LanguageEnum { english = 0, french = 1, german = 2, spanish = 3, catalan = 4, dutch = 5,
                    italian = 6, turkish = 7, portuguese = 8, swedish = 9 };
enum FormatEnum { month_day = 0, day_month = 1 };
#define BLUETOOTH ((char*) 0xE0101A9C)

//  variables

static Window window;
static TextLayer text_date_layer, text_time_layer;
static Layer line_layer;
static InverterLayer inverter_layer;
static PblTm last_time;
enum LanguageEnum language;
enum FormatEnum format;
static char time_text[] = "00:00", date_text[] = "Xxxxxxxxxxxxxxxxxxxxxxx 00", str_url[40];

//  languages - unable to make constant

static char* kMonths[] = {   //  unable to make "const" when more than 6 languages
    "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December",
    "janvier", "février", "mars", "avril", "mai", "juin", "juillet", "août", "septembre", "octobre", "novembre", "décembre",
    "Januar", "Februar", "März", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember",
    "enero", "febrero", "marzo", "abril", "mayo", "junio", "julio", "agosto", "septiembre", "octubre", "noviembre", "diciembre",
    "gener", "febrer", "març", "abril", "maig", "juny", "juliol", "agost", "setembre", "octubre", "novembre", "desembre",
    "januari", "februari", "maart", "april", "mei", "juni", "juli", "augustus", "september", "oktober", "november", "december",
    "Gennaio", "Febbraio", "Marzo", "Aprile", "Maggio", "Giugno", "Luglio", "Agosto", "Settembre", "Ottobre", "Novembre", "Dicembre",
    "Ocak", "Şubat", "Mart", "Nisan", "Mayıs", "Haziran", "Temmuz", "Ağustos", "Eylül ", "Ekim", "Kasım", "Aralık",
    "Janeiro", "Fevereiro", "Março", "Abril", "Maio", "Junho", "Julho", "Agosto", "Setembro", "Outubro", "Novembro", "Dezembro",
    "Januari", "Februari", "Mars", "April", "Maj", "Juni", "Juli", "Augusti", "September", "Oktober", "November", "December"
};

//  functions

void line_layer_update_callback(Layer *me, GContext* ctx) {
  //  set white color
  graphics_context_set_stroke_color(ctx, GColorWhite);
  //  draw horizontal line
  graphics_draw_line(ctx, GPoint(8, 97), GPoint(131, 97));
  graphics_draw_line(ctx, GPoint(8, 98), GPoint(131, 98));
}

void redraw_time(void) {
  //  set date
  *date_text = '\0';
  //  start with day
  if (format == day_month) {
    string_format_time(date_text + strlen(date_text), sizeof(date_text) - strlen(date_text), "%e ", &last_time);
    //  special Catalan format
    if (language == catalan)
      strncpy(date_text + strlen(date_text),
              strchr("aeiou", kMonths[12 * language + last_time.tm_mon][0]) ? "d'" : "de ",
              sizeof(date_text) - strlen(date_text));
  }
  //  display month
  strncpy(date_text + strlen(date_text), kMonths[12 * language + last_time.tm_mon], sizeof(date_text) - strlen(date_text));
  //  add day
  if (format == month_day)
    string_format_time(date_text + strlen(date_text), sizeof(date_text) - strlen(date_text), " %e", &last_time);
  //  display the date
  text_layer_set_text(&text_date_layer, date_text);
  //  set time
  char* time_format = clock_is_24h_style() ? "%R" : "%I:%M";
  string_format_time(time_text, sizeof(time_text), time_format, &last_time);
  //  shift string for preceding space or zero character
  if (!clock_is_24h_style() && ((time_text[0] == ' ') || (time_text[0] == '0')))
    memmove(&time_text[0], &time_text[1], sizeof(time_text) - 1);
  text_layer_set_text(&text_time_layer, time_text);
}

void http_success_handler(int32_t request_id, int http_status, DictionaryIterator* received, void* ctx) {
  //  check cookie
  if (request_id == kCookie) {
    //  extract data
    Tuple* data_tuple = dict_find(received, 1);
    if (data_tuple && strlen(data_tuple->value->cstring)) {
      //  check SetPebble.com response custom setting characters
      if (data_tuple->value->cstring[0] == '1') {
        //  invert screen
        inverter_layer_init(&inverter_layer, GRect(0, 0, 144, 168));
        layer_add_child(window_get_root_layer(&window), &inverter_layer.layer);
      }
      if (data_tuple->value->cstring[1]) {
        char ch = data_tuple->value->cstring[1];
        if ((ch >= '0') && (ch < '0' + kNumLanguages)) {
          //  set new language
          language = (ch - '0');
          //  redraw time
          redraw_time();
        }
      }
      if (strlen(data_tuple->value->cstring) > 2) {
        char ch = data_tuple->value->cstring[2];
        if ((ch >= '0') && (ch < '0' + 2)) {
          //  set new date format
          format = (ch - '0');
          //  redraw time
          redraw_time();
        }
      }
    }
  }
}

void handle_init(AppContextRef ctx) {
  //  initialize window
  window_init(&window, "Simplicity International");
  window_stack_push(&window, true);
  window_set_background_color(&window, GColorBlack);
  //  initialize resources
  resource_init_current_app(&APP_RESOURCES);
  //  date display
  text_layer_init(&text_date_layer, window.layer.frame);
  text_layer_set_text_color(&text_date_layer, GColorWhite);
  text_layer_set_background_color(&text_date_layer, GColorClear);
  layer_set_frame(&text_date_layer.layer, GRect(8, 68, 144-8, 168-68));
  text_layer_set_font(&text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
  layer_add_child(&window.layer, &text_date_layer.layer);
  //  time display
  text_layer_init(&text_time_layer, window.layer.frame);
  text_layer_set_text_color(&text_time_layer, GColorWhite);
  text_layer_set_background_color(&text_time_layer, GColorClear);
  layer_set_frame(&text_time_layer.layer, GRect(7, 92, 144-7, 168-92));
  text_layer_set_font(&text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49)));
  layer_add_child(&window.layer, &text_time_layer.layer);
  //  horizontal line
  layer_init(&line_layer, window.layer.frame);
  line_layer.update_proc = &line_layer_update_callback;
  layer_add_child(&window.layer, &line_layer);
  //  default language
  language = english;
  format = month_day;
  //  SETPEBBLE: register callbacks for httPebble
  http_set_app_id(kAppId);
  http_register_callbacks((HTTPCallbacks) {
    .success = http_success_handler
  }, ctx);
  //  create URL for getting settings
  strcpy(str_url, "http://setpebble.com/api/JRSR/");
  //  start with separator and termination character
  str_url[strlen(str_url) + 12] = '\0';
  //  copy bluetooth address
  memmove(str_url + strlen(str_url), BLUETOOTH, 12);
  //  send request to retrieve custom settings
  DictionaryIterator* body;
  HTTPResult http_result = http_out_get(str_url, kCookie, &body);
  if (http_result == HTTP_OK) {
    http_result = http_out_send();
    if (http_result != HTTP_OK) {
      //  possibly display error here
    }
  }
  //  get and display initial time
  get_time(&last_time);
  redraw_time();
}

void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {
  //  remember time
  last_time = *t->tick_time;
  //  redraw time
  redraw_time();
}

void pbl_main(void *params) {
  //  set handlers
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .tick_info = {
      .tick_handler = &handle_minute_tick,
      .tick_units = MINUTE_UNIT
    },
    .messaging_info = {
      .buffer_sizes = {
        .inbound = 124,
        .outbound = 124,
      }
    }
  };
  app_event_loop(params, &handlers);
}
