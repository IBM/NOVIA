#!/bin/bash
###################################################################################################
# Help
###################################################################################################
Help(){
  # Display Help
  echo "OVERVIEW: Novia Analysis Helper:"
  echo "The helper will create a Novia folder in the location of the bitcode and run 
        the Novia analysis in it."
  echo 
  echo "USAGE: novia [-h|v] <input bitfile> [-c conf] [-p perf] [-r novia_bitcode]"
  echo "bitfile: Path to the llvm-ir bitcode file to analyze."
  echo 
  echo "OPTIONS:"
  echo -e "-h\tPrint this Help."
  echo -e "-v\tVerbose mode."
  echo -e "-t\tTest mode. Generated bitcode with inline accelerators will be validated against 
        output of original bitcode"
  echo -e "-r <novia_bitcode>\tReuse mode. Search already developed inline functions in a new bitcode file"
  echo -e "-c <config file>\tPath to config file with compile and link flags for the 
        application."
  echo -e "-p <performance threshold>\tNumber of basic blocks to analyze.
        If number > 1; Number of basic blocks to analyze. If perf < 1; Number of 
        basic blocks = perf % of bitcode execution time to analyze."
}
###################################################################################################
# Main
###################################################################################################
# Parse Options
###################################################################################################
VERBOSE=0
TEST=0
BITCODE=""
MATCH=""
while [[ $# -gt 0 ]]; do
  case $1 in
    -h) # Display Help
      Help
      exit 0;;
    -v) # Verbose mode
      VERBOSE=1
      shift
      ;;
    -t) # Test mode
      TEST=1
      shift
      ;;
    -c) # Config file
      CONFIG="$2"
      shift
      shift
      ;;
    -p) # Perf threshold
      PERF="$2"
      shift
      shift
      ;;
    -r) # Matching Mode bc
      MATCH="$2"
      shift
      shift
      ;;
    -*) # Unrecognized option
      echo "Error: Unrecognized option $1"
      exit 1;;
    *)
      BITCODE="$1" # save positional arg
      shift 
      ;;
  esac
done
###################################################################################################
# Input error check
###################################################################################################
if [ -z "$BITCODE" ]; then
  echo "Invalid Arguments: Missing llvm-ir source bitcode file"
  Help
  exit 1
else
  if [ ! -f "$BITCODE" ]; then
    echo "Invalid Arguments: Missing or Unable to open bitcode file ($BITCODE)"
    exit 1
  fi
  BITCODE_PATH=$(dirname -- "$(readlink -f -- "$BITCODE")")
  if [ -z $CONFIG ]; then
    CONFIG=conf.sh
    if [ ! -f $CONFIG ]; then
      echo "Invalid Arguments: Missing or Unable to open default compile config file ($CONFIG)"
      exit 1
    fi
  else
    if [ ! -f "$CONFIG" ]; then
      echo "Invalid Arguments: Missing or Unable to open compile config file ($CONFIG)"
      exit 1
    fi
  fi
fi
###################################################################################################
# Novia Analysis 
###################################################################################################
scriptDir=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")
source $scriptDir/../../../env.sh
source $scriptDir/../../../novia.conf

if [ -z $MATCH ]; then
  if [ ! -z $VERBOSE ]; then
    echo -e "Starting Novia Analysis:"
  fi
  if [ -z $PERF ]; then
    if [ ! -z $VERBOSE ]; then
      echo -e "Using default performance threshold (0.8)"
    fi
    PERF=0.8
  fi
else
  if [ ! -z $VERBOSE ]; then
    echo -e "Starting Novia Reuse Analysis:"
  fi
fi


if [ -z $MATCH ]; then
  mkdir -p $BITCODE_PATH/novia
  cp $BITCODE "$BITCODE_PATH/novia/."
  cd $BITCODE_PATH/novia
  source $BITCODE_PATH/$CONFIG
  source $scriptDir/analyze.sh $BITCODE "$LDFLAGS" "$EXECARGS" $PERF y
  ### Test output bitcode
  if [ ! -z $TEST ]; then
    source $scriptDir/testing.sh 
  fi
else
  source $scriptDir/reuse.sh $MATCH $BITCODE
fi

