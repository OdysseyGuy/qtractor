#pragma once

#include "private/macros.h"
#include "host.h"
#include "process.h"
#include "plugin-features.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct clap_plugin_descriptor {
   clap_version_t clap_version; // initialized to CLAP_VERSION

   // Mandatory fields must be set and must not be blank.
   // Otherwise the fields can be null or blank, though it is safer to make them blank.
   const char *id;          // eg: "com.u-he.diva", mandatory
   const char *name;        // eg: "Diva", mandatory
   const char *vendor;      // eg: "u-he"
   const char *url;         // eg: "https://u-he.com/products/diva/"
   const char *manual_url;  // eg: "https://dl.u-he.com/manuals/plugins/diva/Diva-user-guide.pdf"
   const char *support_url; // eg: "https://u-he.com/support/"
   const char *version;     // eg: "1.4.4"
   const char *description; // eg: "The spirit of analogue"

   // Arbitrary list of keywords.
   // They can be matched by the host indexer and used to classify the plugin.
   // The array of pointers must be null terminated.
   // For some standard features see plugin-features.h
   const char *const *features;
} clap_plugin_descriptor_t;

typedef struct clap_plugin {
   const clap_plugin_descriptor_t *desc;

   void *plugin_data; // reserved pointer for the plugin

   // Must be called after creating the plugin.
   // If init returns false, the host must destroy the plugin instance.
   // [main-thread]
   bool(CLAP_ABI *init)(const struct clap_plugin *plugin);

   // Free the plugin and its resources.
   // It is required to deactivate the plugin prior to this call.
   // [main-thread & !active]
   void(CLAP_ABI *destroy)(const struct clap_plugin *plugin);

   // Activate and deactivate the plugin.
   // In this call the plugin may allocate memory and prepare everything needed for the process
   // call. The process's sample rate will be constant and process's frame count will included in
   // the [min, max] range, which is bounded by [1, INT32_MAX].
   // Once activated the latency and port configuration must remain constant, until deactivation.
   //
   // [main-thread & !active_state]
   bool(CLAP_ABI *activate)(const struct clap_plugin *plugin,
                            double                    sample_rate,
                            uint32_t                  min_frames_count,
                            uint32_t                  max_frames_count);
   // [main-thread & active_state]
   void(CLAP_ABI *deactivate)(const struct clap_plugin *plugin);

   // Call start processing before processing.
   // [audio-thread & active_state & !processing_state]
   bool(CLAP_ABI *start_processing)(const struct clap_plugin *plugin);

   // Call stop processing before sending the plugin to sleep.
   // [audio-thread & active_state & processing_state]
   void(CLAP_ABI *stop_processing)(const struct clap_plugin *plugin);

   // - Clears all buffers, performs a full reset of the processing state (filters, oscillators,
   //   enveloppes, lfo, ...) and kills all voices.
   // - The parameter's value remain unchanged.
   // - clap_process.steady_time may jump backward.
   //
   // [audio-thread & active_state]
   void(CLAP_ABI *reset)(const struct clap_plugin *plugin);

   // process audio, events, ...
   // All the pointers coming from clap_process_t and its nested attributes,
   // are valid until process() returns.
   // [audio-thread & active_state & processing_state]
   clap_process_status(CLAP_ABI *process)(const struct clap_plugin *plugin,
                                          const clap_process_t     *process);

   // Query an extension.
   // The returned pointer is owned by the plugin.
   // [thread-safe]
   const void *(CLAP_ABI *get_extension)(const struct clap_plugin *plugin, const char *id);

   // Called by the host on the main thread in response to a previous call to:
   //   host->request_callback(host);
   // [main-thread]
   void(CLAP_ABI *on_main_thread)(const struct clap_plugin *plugin);
} clap_plugin_t;

#ifdef __cplusplus
}
#endif
