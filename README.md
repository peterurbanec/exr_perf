Change in performance, measured with backported version of exrmetrics from this repository
Quantity measured is time taken for one frame, therefore positive Δ % quantities indicate
a performance regression. These figures were collected on AMD Ryzen Threadripper PRO 3975WX
running Linux


```
exrmetrics          3.2.1           3.3.1                 3.2.1            3.3.1
              Encode time     Encode time    Δ %    Decode time      Decode time     Δ %
none-half       0.0506104       0.0538534     6%      0.0147626       0.00607923    -59%
none-float      0.1009070       0.1006210    -0%      0.0210363       0.00938430    -55%
rle-half        0.0742712       0.0735408    -1%      0.0315694       0.02286890    -28%
rle-float       0.1433720       0.1428730    -0%      0.0468555       0.03499330    -25%
zips-half       0.1403540       0.1435510     2%      0.0413822       0.03292240    -20%
zips-float      0.2781060       0.2841830     2%      0.0759776       0.06599230    -13%
zip-half        0.1490650       0.1478110    -1%      0.0362129       0.02737780    -24%
zip-float       0.3176970       0.3146880    -1%      0.0757593       0.06454440    -15%
piz-half        0.1664580       0.1329990   -20%      0.0470330       0.04469030     -5%
piz-float       0.5811040       0.5301250    -9%      0.1553520       0.11521600    -26%
pxr24-half      0.1577990       0.1601900     2%      0.0300827       0.02612540    -13%
pxr24-float     0.2518220       0.2486510    -1%      0.0582780       0.03983260    -32%
b44-half        0.0790941       0.0811910     3%      0.0132777       0.01222920     -8%
b44-float       0.1036020       0.1121230     8%      0.0210958       0.00707329    -67%
b44a-half       0.0723179       0.0742420     3%      0.0113973       0.01050320     -8%
b44a-float      0.1024170       0.1032410     0%      0.0214426       0.00713971    -67%
dwaa-half       0.2754550       0.2919960     6%      0.0386784       0.04521370     17%  *
dwaa-float      0.3004660       0.3176720     6%      0.0591789       0.06652650     12%  *
dwab-half       0.2452280       0.2807360    15%      0.0395200       0.04543520     15%  *
dwab-float      0.2713410       0.2937500     8%      0.0637925       0.06695870      5%  *

```

Change in performance as observed in a real-world application. Quantity measured is frames
per second, therefore a negative Δ % is a performance regression. These figures were collected
on i7-13800H running Windows.

```
                         3.2.1        3.3.1               3.2.1       3.3.1        
                    Encode fps   Encode fps    Δ %   Decode fps  Decode fps    Δ % 
 HD-half-DWA32(45)        28.2         11.8    -58         71.8        66.2     -8  *
 HD-float-DWA32(45)       16.7          9.0    -46         31.0        35.9     15  *
 HD-half-DWA256(45)       16.4         14.8    -10         43.1        38.7    -10  *
 HD-float-DWA256(45)       9.4          9.2     -2         21.7        24.5    -13  *
 HD-half-piz              31.7         29.7     -6         64.4        58.6     -9  *
 HD-half-Pxr24            19.2         18.9     -2         79.5        73.6     -7
 HD-float-Pxr24           20.4         24.2     19         39.6        43.2      9
 HD-half-B44A             19.3         19.8      3         70.8        68.2     -3 
 HD-float-B44A            17.7         18.1      2         32.4        40.7     26 

```

Rows marked with * are of particular concern.
