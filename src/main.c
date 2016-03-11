#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_output_layer;

int sMinX = 10000;
int sMinY = 10000;
int sMinZ = 10000;
int h = 500;
int fall = 0;

int calculateS(int accelerometerValue);
int decideFall(int s, int axis);
static void fall_found();
void down_single_click_handler(ClickRecognizerRef recognizer, void *context);
void up_single_click_handler(ClickRecognizerRef recognizer, void *context);
void select_single_click_handler(ClickRecognizerRef recognizer, void *context);
void contact_android();

static void data_handler(AccelData *data, uint32_t num_samples) {
  // Long lived buffer
  static char s_buffer[128];
  
  int x = calculateS(data[0].x);
  int y = calculateS(data[0].y);
  int z = calculateS(data[0].z);
  
  int fallX = decideFall(x, 1); // axis == 1 for X 
  int fallY = decideFall(y, 1); // axis == 1 for Y
  int fallZ = decideFall(z, 1); // axis == 1 for Z 
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "x axis now %d == %d", data[0].x, x);
  // Compose string of all data
  snprintf(s_buffer, sizeof(s_buffer), 
    "N X,Y,Z\n0 %d,%d,%d\n1 %d,%d,%d\n2 %d,%d,%d,\n,%d", 
    data[0].x, data[0].y, data[0].z, 
    data[1].x, data[1].y, data[1].z, 
    data[2].x, data[2].y, data[2].z,x
  );
  text_layer_set_text(s_output_layer, s_buffer);

  if(fallX ==1 || fallY ==1 || fallZ ==1 || fall ==1){
    fall = 1;
    accel_data_service_unsubscribe();
    APP_LOG(APP_LOG_LEVEL_DEBUG, " FALL ");
    snprintf(s_buffer, sizeof(s_buffer),"FALL");
    text_layer_set_text(s_output_layer, s_buffer);
    fall_found();
  }
}

static void fall_found(){
  APP_LOG(APP_LOG_LEVEL_DEBUG, " FALL detected and into fall found function ");
   static char new_buffer[128];
   snprintf(new_buffer, sizeof(new_buffer),"you have fallen, do you need help ?");
   text_layer_set_text(s_output_layer, new_buffer);
}

int calculateS(int accelerometerValue){
  int theta0 = 0;
  int theta1 = 1;
  int sigma = 2;
  
  //s will be multiplied by 1000 due to problems with floating points. divided into two calculations
  //s = (theta0 - theta1)/(sigma * sigma) * (accelerometerValue - (theta0 - theta1)/2);
  int first  = (theta0 - theta1) * 1000 / (sigma * sigma);
  int second = (accelerometerValue - (theta0 - theta1)/2);
  int s = first * second;
//   APP_LOG(APP_LOG_LEVEL_DEBUG, " X %d into the decideFall function %d ",accelerometerValue,s);
  return s;
}

int decideFall(int s, int axis){
  int decideFall = 0;   //starts for fall = 0, no fall.
  int sMin = 0;
  if( axis == 1){
    if (s < sMinX){
      sMinX = s;
    }
    sMin = sMinX;
  }else if (axis == 2){
    if (s < sMinY){
      sMinY = s;
    }    
    sMin = sMinY;
  }else if ( axis == 3){
    if (s < sMinZ){
      sMinZ = s;
    }
    sMin = sMinZ;
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, " Smin ---> %d , S ---> %d ",sMin,s);
  if(s/1000 - sMin/1000 > h){
    decideFall = 1;
  }
  
  return decideFall;
}

void config_provider(Window *window) {
 // single click / repeat-on-hold config:
  window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
}

void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  contact_android();
}

void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  contact_android();
}

void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  contact_android();
}

void contact_android(){
  if(fall == 1){
    APP_LOG(APP_LOG_LEVEL_DEBUG, "*********** CONTACT ANDROID APP *************");
  }else{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "########### FALL NOT YET DETECTED ##############");
  }
}


static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  // Create output TextLayer
  s_output_layer = text_layer_create(GRect(5, 0, window_bounds.size.w - 10, window_bounds.size.h));
  text_layer_set_font(s_output_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text(s_output_layer, "No data yet.");
  text_layer_set_overflow_mode(s_output_layer, GTextOverflowModeWordWrap);
  layer_add_child(window_layer, text_layer_get_layer(s_output_layer));
}

static void main_window_unload(Window *window) {
  // Destroy output TextLayer
  text_layer_destroy(s_output_layer);
}

static void init() {
  // Create main Window
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
  // Subscribe to the accelerometer data service
  int num_samples = 3;
  accel_data_service_subscribe(num_samples, data_handler);

  // Choose update rate
  accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
  window_set_click_config_provider(s_main_window, (ClickConfigProvider) config_provider);

}

static void deinit() {
  // Destroy main Window
  window_destroy(s_main_window);


  accel_data_service_unsubscribe();
  
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}