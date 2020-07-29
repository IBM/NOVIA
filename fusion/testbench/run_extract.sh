echo "extracting simplbr"
(cd simplebr && ./extract.sh)
echo "extracting simplbr3"
(cd simplebr3 && ./extract.sh)
echo "extracting simplbr_nofuse"
(cd simplebr_nofuse && ./extract.sh)
echo "extracting multidep"
(cd multidep && ./extract.sh)
echo "extracting deploop"
(cd deploop && ./extract.sh)
echo "extracting deploop3"
(cd deploop3 && ./extract.sh)
echo "extracting storeorder"
(cd storeorder  && ./extract.sh)
echo "extracting incremental"
(cd incremental  && ./extract.sh)
