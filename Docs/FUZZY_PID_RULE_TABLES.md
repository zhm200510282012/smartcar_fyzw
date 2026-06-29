# Fuzzy PID Rule Tables

## Language Variables

Inputs use five language variables:

| Term | Meaning | Center |
| --- | --- | ---: |
| `NB` | Negative Big | -1000 |
| `NS` | Negative Small | -500 |
| `ZO` | Zero | 0 |
| `PS` | Positive Small | 500 |
| `PB` | Positive Big | 1000 |

The edge terms are shoulder-like at `-1000` and `+1000`; inner regions are linear triangular interpolation. Each input activates at most two adjacent terms.

## Rule Operation

For each active `(e, de)` pair:

1. activation strength = `min(weight_e, weight_de)`;
2. rule output is a signed permille of that track mode's adjustment limit;
3. defuzzification is weighted average;
4. `Delta Kp`, `Delta Ki`, `Delta Kd` are clamped to their mode limits;
5. final `Kp/Ki/Kd` are clamped to global min/max.

## Rule Tables

Rows are `e`, columns are `de`, values are permille of the per-mode adjustment limit.

`Delta Kp`:

| e \ de | NB | NS | ZO | PS | PB |
| --- | ---: | ---: | ---: | ---: | ---: |
| NB | 900 | 780 | 640 | 780 | 900 |
| NS | 680 | 520 | 340 | 520 | 680 |
| ZO | 280 | 120 | 0 | 120 | 280 |
| PS | 680 | 520 | 340 | 520 | 680 |
| PB | 900 | 780 | 640 | 780 | 900 |

`Delta Ki`:

| e \ de | NB | NS | ZO | PS | PB |
| --- | ---: | ---: | ---: | ---: | ---: |
| NB | -1000 | -900 | -700 | -900 | -1000 |
| NS | -800 | -550 | -250 | -550 | -800 |
| ZO | -500 | -180 | 200 | -180 | -500 |
| PS | -800 | -550 | -250 | -550 | -800 |
| PB | -1000 | -900 | -700 | -900 | -1000 |

`Delta Kd`:

| e \ de | NB | NS | ZO | PS | PB |
| --- | ---: | ---: | ---: | ---: | ---: |
| NB | 950 | 760 | 520 | 760 | 950 |
| NS | 850 | 620 | 360 | 620 | 850 |
| ZO | 720 | 450 | 160 | 450 | 720 |
| PS | 850 | 620 | 360 | 620 | 850 |
| PB | 950 | 760 | 520 | 760 | 950 |

Ki is effectively off in all non-straight modes because those modes use `ki_adjust_limit = 0` and `base_ki = 0`.
