#include <pebble.h>

Window *window;
TextLayer *text_layer;
static char msg_id[64];
static char msg_name[64];
static char msg_bulb[64];
int bulb;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static AppSync sync;
static uint8_t sync_buffer[128];

#define perma_ID 1
#define perma_Name 2
#define perma_Bulb 3
#define MyTupletCString(_key, _cstring) \
((const Tuplet) { .type = TUPLE_CSTRING, .key = _key, .cstring = { .data = _cstring, .length = strlen(_cstring) + 1 }})

void send_url(char *url)
{
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
 
    Tuplet value = TupletCString(0x3, url);
    dict_write_tuplet(iter, &value);
 
    app_message_outbox_send();
}

void sync_tuple_changed_callback(const uint32_t key,
        const Tuple* new_tuple, const Tuple* old_tuple, void* context) {

  if ((int)new_tuple->key == 0x0){
      memcpy(msg_id, new_tuple->value->cstring, new_tuple->length);
  }
  else if ((int)new_tuple->key == 0x1){
    memcpy(msg_name, new_tuple->value->cstring, new_tuple->length);
    }
  else if ((int)new_tuple->key == 0x2){
    memcpy(msg_bulb, new_tuple->value->cstring, new_tuple->length);
  }
   //APP_LOG(APP_LOG_LEVEL_DEBUG, "%i, %s", (int)new_tuple->key , new_tuple->value->cstring);
}
void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
    // An error occured!
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", dict_error);
}

static void data_handler(AccelData *data, uint32_t num_samples) {
  // Long lived buffer
  static char s_buffer[128];
  static char t_buffer[128];
  int brigt;
  int hue;
  hue = (int) (data[0].y/2);
  if (hue > 254)
    hue = 254;
  if (hue < 0)
    hue = 0;
  if (strcmp(msg_bulb,"1") != 0)
    hue = -1;
  
  brigt = (int) (data[0].x/2);
  if (brigt >0) // point down
    brigt= 0;
  if (brigt <0) // point up
    brigt= brigt*(-1); // translate to positive brigthness
  if (brigt>254) // limit to max brigthness
    brigt = 254;
  // Compose string of all data
  snprintf(s_buffer, sizeof(s_buffer), 
           "http://%s/api/%s/lights/%i/state?%i?%i",msg_id,msg_name,bulb, brigt, hue
  );
  snprintf(t_buffer, sizeof(t_buffer), "%i Huequeen %i", bulb, brigt);
  text_layer_set_text(text_layer, t_buffer);
  send_url(s_buffer);
}


static void up_click(ClickRecognizerRef recognizer, void *context) {
  // A single click has just occured
  bulb = bulb +1;
}
static void down_click(ClickRecognizerRef recognizer, void *context) {
  // A single click has just occured
  bulb = bulb -1;
  if (bulb < 1)
    bulb = 1;
}
static void click_config_provider(void *context) {
  ButtonId id = BUTTON_ID_UP;  // The Select button
  ButtonId id2 = BUTTON_ID_DOWN;  // The Select button
  window_single_click_subscribe(id, up_click);
  window_single_click_subscribe(id2, down_click);
}
void handle_init(void) {
	// Create a window and text layer
  bulb = 1;
	window = window_create();
	text_layer = text_layer_create(GRect(0, 0, 144, 40));
	// Use this provider to add button click subscriptions
  window_set_click_config_provider(window, click_config_provider);
  
	// Set the text, font, and text alignment
	text_layer_set_text(text_layer, "Starting...");
	text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  
    // Create GBitmap, then set to created BitmapLayer
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_img_queen);
  s_background_layer = bitmap_layer_create(GRect(40, 40, 64, 64));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
  
	// Add the text layer to the window
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_layer));

	// Push the window
	window_stack_push(window, true);
	
	// App Logging!
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Just pushed a window!");
  
    // Subscribe to the accelerometer data service
  int num_samples = 3;
  accel_data_service_subscribe(num_samples, data_handler);
  // Choose update rate
  accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
  //memcpy(msg_id," ",1);
  // load user config from persistend data and sync with config
     if (persist_exists(perma_ID)) {   
   persist_read_string(perma_ID, msg_id, 30);
     APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", msg_id);
    }; 
     if (persist_exists(perma_Name)) {   
   persist_read_string(perma_Name, msg_name, 50);
       APP_LOG(APP_LOG_LEVEL_DEBUG, "%s",msg_name);
    }; 
     if (persist_exists(perma_Bulb)) {   
   persist_read_string(perma_Bulb, msg_bulb, 30);
       APP_LOG(APP_LOG_LEVEL_DEBUG, "%s",msg_bulb);
    }; 

    // Setup AppSync
  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);

     Tuplet initial_values[] = {
     MyTupletCString(0x0, msg_id),
       MyTupletCString(0x1, msg_name),
       MyTupletCString(0x2, msg_bulb),
  }; 
  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);
}

void handle_deinit(void) {
  persist_write_string(perma_ID, msg_id);
  persist_write_string(perma_Name, msg_name);
  persist_write_string(perma_Bulb, msg_bulb);
   APP_LOG(APP_LOG_LEVEL_DEBUG, "writing");
  APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", msg_id);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", msg_name);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", msg_bulb);
	// Destroy the text layer
	text_layer_destroy(text_layer);
	// Destroy BitmapLayer
  gbitmap_destroy(s_background_bitmap);
  bitmap_layer_destroy(s_background_layer);
  
	// Destroy the window
	window_destroy(window);
  
  // Destroy sensor
  accel_data_service_unsubscribe();
  
  // Finish using AppSync
  app_sync_deinit(&sync);

}

int main(void) {
	handle_init();
	app_event_loop();
	handle_deinit();
}
