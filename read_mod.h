#ifndef ARCTRACKER_READ_MOD_H
#define ARCTRACKER_READ_MOD_H

#define READONLY "r"

module_t read_file();

module_t read_tracker_file(void *p_modfile, long p_modsize);

module_t read_desktop_tracker_file(void *p_modfile);

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
        bool p_null_term);

void read_nbytes(
        long *p_output,
        void *p_input,
        int p_num_bytes);

void get_patterns(void *p_search_from, long p_array_end, void **p_patterns);

int get_samples(void *p_search_from, long p_array_end, sample_t *p_samples);

return_status get_sample_info(
        void *p_search_from,
        long p_array_end,
        sample_t *p_sample);

#endif //ARCTRACKER_READ_MOD_H
