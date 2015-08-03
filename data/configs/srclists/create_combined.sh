#!/bin/bash
cat k_names.txt k_address.txt k_misc.txt > k_geocell.txt
cat k_names.txt k_address.txt > k_sortorder.txt
cat k_geocell.txt k_addradminlevel.txt > k_itemsearch.txt
cat k_itemsearch.txt k_poi.txt > k_db_reduced.txt
