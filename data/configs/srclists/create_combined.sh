#!/bin/bash
cat k_names.txt k_address.txt > k_values_substring_prefix.txt
cat k_misc.txt > k_values_prefix.txt
cat k_values_substring_prefix.txt k_values_prefix.txt k_poi.txt > k_db_reduced.txt
