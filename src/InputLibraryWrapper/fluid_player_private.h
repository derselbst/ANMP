
// fluidsynths private struct it uses for fluid_player_t
// we need to access cur_msec and begin_msec to determine playtime of a midi file

struct _fluid_player_t {
  int status;
  int ntracks;
  void *track[128];
  fluid_synth_t* synth;
  void* system_timer;
  void* sample_timer;

  int loop; /* -1 = loop infinitely, otherwise times left to loop the playlist */
  void* playlist; /* List of fluid_playlist_item* objects */
  void* currentfile; /* points to an item in files, or NULL if not playing */

  char send_program_change; /* should we ignore the program changes? */
  char use_system_timer;   /* if zero, use sample timers, otherwise use system clock timer */
  char reset_synth_between_songs; /* 1 if system reset should be sent to the synth between songs. */
  int start_ticks;          /* the number of tempo ticks passed at the last tempo change */
  int cur_ticks;            /* the number of tempo ticks passed */
  int begin_msec;           /* the time (msec) of the beginning of the file */
  int start_msec;           /* the start time of the last tempo change */
  int cur_msec;             /* the current time */
  int miditempo;            /* as indicated by MIDI SetTempo: n 24th of a usec per midi-clock. bravo! */
  double deltatime;         /* milliseconds per midi tick. depends on set-tempo */
  unsigned int division;

  handle_midi_event_func_t playback_callback; /* function fired on each midi event as it is played */
  void* playback_userdata; /* pointer to user-defined data passed to playback_callback function */
}; 
