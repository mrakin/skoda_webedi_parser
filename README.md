Tool for download of Skoda Webedi orders/references

This application simmulates user interaction with https://web3.teledin.cz/WebEdi2. You need to provide correct credentials via configuration.xml file, which is auto-generated after first run.

The output of parser are 3 csv files:

    Output/YYYYMMDD_hhmm_all.csv - contains all the references for all the parts
    Output/YYYYMMDD_hhmm.csv - contains only references, which still has non-zero value for number of requested parts
    Output/last.csv - copy of the file above

Following options are available: Syntax: DelforParser [-d] [-h | -v | -s] [-m] -d Enable debug messages -h Print help and exit -v Print version and exit -s Create category file

By modification of formatting.xml file you can select which fields, in which order and what column name will be displayed in the csv files.

After the category file is generated you can define group (1 and 2) for all the parts to apply filters easily on the csv files.

Application requires libcurl library. This version is the first remake of original quick & dirty C-written solution."# skoda_webedi_parser" 
