#include <pebble.h>

#define NUM_QUESTIONS 5

static Window *s_main_window;
static TextLayer *s_question_layer, *s_prompt_layer;

static DictationSession *s_dictation_session;
static char s_last_text[256];

static char s_questions[5][64] = {
  "Which animal has a long trunk?",
  "What color is the sky?",
  "Which city is the capital of the UK?",
  "Who was the first man on the Moon?",
  "In which state is Silicon Valley?"
};

static char s_answers[5][32] = {
  "lephant",  // E/elephant
  "lue",      // Blue/blue
  "London",
  "Armstrong",
  "California"
};

static int s_current_question, s_correct_answers;
static bool s_speaking_enabled;

/********************************* Quiz Logic *********************************/

static void next_question_handler(void *context) {
  if(s_current_question == NUM_QUESTIONS) {
    // Quiz is over
    window_set_background_color(s_main_window, GColorDukeBlue);
    text_layer_set_text(s_question_layer, "Quiz Finished!");

    static char s_result_buff[32];
    snprintf(s_result_buff, sizeof(s_result_buff), "You got %d of %d correct!", 
             s_correct_answers, NUM_QUESTIONS);
    text_layer_set_text(s_prompt_layer, s_result_buff);
  } else {
    // Next question
    text_layer_set_text(s_question_layer, s_questions[s_current_question]);
    text_layer_set_text(s_prompt_layer, "Press Select to speak your answer!");
    window_set_background_color(s_main_window, GColorDarkGray);
    s_speaking_enabled = true;
  }
}

static void check_answer(char *answer) {
  bool correct = strstr(answer, s_answers[s_current_question]);

  correct ? vibes_double_pulse() : vibes_long_pulse();
  text_layer_set_text(s_question_layer, correct ? "Correct!" : "Wrong!");
  text_layer_set_text(s_prompt_layer, (s_current_question == NUM_QUESTIONS - 1) 
                      ? "" : "Here comes the next question...");
  window_set_background_color(s_main_window, correct ? GColorGreen : GColorRed);
  s_correct_answers += correct ? 1 : 0;

  s_current_question++;

  app_timer_register(3000, next_question_handler, NULL);
}

/******************************* Dictation API ********************************/

static void dictation_session_callback(DictationSession *session, DictationSessionStatus status, 
                                       char *transcription, void *context) {
  if(status == DictationSessionStatusSuccess) {
    // Check this answer
    check_answer(transcription);
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Transcription failed.\n\nError ID:\n%d", (int)status);
  }
}

/************************************ App *************************************/

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(s_speaking_enabled) {
    // Start voice dictation UI
    dictation_session_start(s_dictation_session);
    s_speaking_enabled = false;
  }
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_question_layer = text_layer_create(GRect(5, 5, bounds.size.w - 10, bounds.size.h));
  text_layer_set_font(s_question_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_color(s_question_layer, GColorWhite);
  text_layer_set_text_alignment(s_question_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_question_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_question_layer));

  s_prompt_layer = text_layer_create(GRect(5, 120, bounds.size.w - 10, bounds.size.h));
  text_layer_set_text(s_prompt_layer, "Press Select to speak your answer!");
  text_layer_set_font(s_prompt_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_color(s_prompt_layer, GColorWhite);
  text_layer_set_text_alignment(s_prompt_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_prompt_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_prompt_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(s_prompt_layer);
  text_layer_destroy(s_question_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_main_window, true);

  // Create new dictation session
  s_dictation_session = dictation_session_create(sizeof(s_last_text), 
                                                 dictation_session_callback, NULL);

  window_set_background_color(s_main_window, GColorDarkGray);
  s_current_question = 0;
  text_layer_set_text(s_question_layer, s_questions[s_current_question]);
  s_speaking_enabled = true;
}

static void deinit() {
  // Free the last session data
  dictation_session_destroy(s_dictation_session);

  window_destroy(s_main_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
