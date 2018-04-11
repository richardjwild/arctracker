#ifndef ARCTRACKER_READ_MOD_H
#define ARCTRACKER_READ_MOD_H

return_status read_file(
        void *p_modfile,
        long p_modsize,
        mod_details *p_module,
        sample_details *p_samples);

return_status read_tracker_file(
        void *p_modfile,
        long p_modsize,
        mod_details *p_module,
        sample_details *p_samples);

return_status read_desktop_tracker_file(
        void *p_modfile,
        mod_details *p_module,
        sample_details *p_samples);

return_status search_tff(
        void *p_searchfrom,
        void **p_chunk_address,
        long p_array_end,
        char *p_chunk,
        long p_occurrence);

void read_nchar(
        char *p_output,
        void *p_input,
        int p_num_chars,
        yn p_null_term);

void read_nbytes(
        long *p_output,
        void *p_input,
        int p_num_bytes);

return_status get_patterns(
        void *p_search_from,
        long p_array_end,
        void **p_patterns,
        int *p_num_patterns);

return_status get_samples(
        void *p_search_from,
        long p_array_end,
        int *p_samples_found,
        sample_details *p_samples);

return_status get_sample_info(
        void *p_search_from,
        long p_array_end,
        sample_details *p_sample);

#endif //ARCTRACKER_READ_MOD_H
