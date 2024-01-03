# Wiener Netze Smart Meter Date description

| Parameter                                          | Length | Example          | Value                                 |
| -------------------------------------------------- | ------ | ---------------- | ------------------------------------- |
| Prefix                                             |  4     | 02 09 09 0c      | -                                     |
| Date (YYY-MM-DD)                                   |  4     | 07 e7 0b 0a      | 2023-11-10                            |
| Time (WD HH:MM:SS)                                 |  4     | 05 12 19 11      | Fr. 18:25:11                          |
| unknown                                            |  5     | ff 80 00 00 *06* | 4 bytes seem constant + separator 06  |
| A+ (energy consumption kWh) U32 (3 decimal places) |  5     | 00 10 2c 7a *06* | 1059.962 kWh + separator 06           |
| A- (energy production kWh)  as A+                  |  5     | 00 00 00 00 *06* | 0.000 kWh + separator 06              |
| R+ (blind energy consumption kVAh) as A+           |  5     | 00 02 9c d4 *06* | 171.22 kVAh + separator 06            |
| R- (blind energy generated kVAh)   as A+           |  5     | 00 02 15 1b *06* | 136.475 kVAh + separator 06           |
| P+ (currrent power used W) as A+                   |  5     | 00 33 ba 30 *06* | 3390 W + separator 06                 |
| P- (current power produced W) as A+                |  5     | 00 00 00 00 *06* | 0 W + separator 06                    |
| Q+ (current blind power used W) as A+              |  5     | 00 00 00 1b *06* | 27 W + separator 06                   |
| Q- (current blind power generated W) as A-         |  5     | 00 00 00 52 *06* | 82 W + separator 06                   

## script:

````
>D
>B
=>sensor53 r
>M 1
+1,3,r,0,9600,Home
1,=so3,256
1,=so4,<DECRYPT_KEY>
1,0209090cUUuu@1,year,,year,0
1,0209090cx2ss@1,month,,month,0
1,0209090cx3ss@1,day,,day,0
1,0209090cx5ss@1,hh,,hh,0
1,0209090cx6ss@1,mm,,mm,0
1,0209090cx7ss@1,ss,,ss,0
1,0209090cx13UUuuUUuu@1000,+A,kWh,+A,3
1,0209090cx18UUuuUUuu@1000,-A,kWh,-A,3
1,0209090cx23UUuuUUuu@1000,+R,varh,+R,3
1,0209090cx28UUuuUUuu@1000,-R,varh,-R,3
1,0209090cx33UUuuUUuu@1,+P,W,+P,3 
1,0209090cx38UUuuUUuu@1,-P,W,-P,3
1,0209090cx43UUuuUUuu@1,+Q,var,+Q,3 
1,0209090cx48UUuuUUuu@1,-Q,var,-Q,3
#
````


{
  "Time": "2023-11-11T19:55:49",
  "Home": {
    "year": 2023,
    "month": 11,
    "day": 11,
    "hh": 19,
    "mm": 55,
    "ss": 52,
    "+A": 1083.344,
    "-A": 0,
    "+R": 171.22,
    "-R": 137.946,
    "+P": 4627,
    "-P": 0,
    "+Q": 27,
    "-Q": 82
  }
}


{
  "Time": "2023-11-11T19:54:19",
  "Home": {
    "year": 2023,
    "month": 11,
    "day": 11,
    "hh": 19,
    "mm": 54,
    "ss": 21,
    "+A": 1083.243,
    "-A": 0,
    "+R": 171.22,
    "-R": 137.945,
    "+P": 3390,
    "-P": 0,
    "+Q": 27,
    "-Q": 82
  }
}