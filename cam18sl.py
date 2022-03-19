import math
import numpy as np

p = 0.58

def cam18sl_cone_compress(x, a):
    return math.pow(x, p) / (math.pow(x, p) + math.pow(a, p))

def cam18sl_cone_compress_inverse(x, a):
    return math.pow((x * math.pow(a, p)) / (1.0 - x), 1.0/p)

# Opponent process represented as a matrix
LMSc_to_Aab = np.array([
    np.array([2.0,        1.0,  1.0/20.0]),
    np.array([1.0, -12.0/11.0,  1.0/11.0]) * 0.63,
    np.array([1.0,        1.0,      -2.0]) * 0.12
])

# colourfulness
def cam18sl_M(a, b):
    return 3260.0 * math.sqrt(a*a + b*b)

# Brightness
def cam18sl_Q(a, b, A):
    return 0.937 * (A + 0.0024 * cam18sl_M(a,b)**1.09);

# Luminance-ish
def cam18sl_A(a, b, Q):
    return Q / 0.937 - 0.0024 * cam18sl_M(a,b)**1.09

def cam18sl(L, M, S, BackgroundValue):
    a = (291.2 + 71.8*math.pow(BackgroundValue, 0.78))
    LMS_c = [
        cam18sl_cone_compress(L, a),
        cam18sl_cone_compress(M, a),
        cam18sl_cone_compress(S, a)
    ]

    Aab = np.matmul(LMSc_to_Aab, LMS_c)

    return {
        "A" : Aab[0],
        "a" : Aab[1],
        "b" : Aab[2],
        "M" : cam18sl_M(Aab[1], Aab[2]),
        "Q" : cam18sl_Q(Aab[1], Aab[2], Aab[0])
    }

def cam18sl_inverse(BackgroundValue, a, b, A, Q):
    Aab_to_LMSc = np.linalg.inv(LMSc_to_Aab)
    Aab = [A, a, b]

    # If no A (lumninanceish) signal, get it out from brightness (Q)
    if (A == None):
        Aab[0] = cam18sl_A(a, b, Q)

    # Get back to LMS compressed signal
    LMS_c = np.matmul(Aab_to_LMSc, Aab)

    # Uncompress LMS signal
    _a = (291.2 + 71.8*math.pow(BackgroundValue, 0.78))
    return [
        cam18sl_cone_compress_inverse(LMS_c[0], _a),
        cam18sl_cone_compress_inverse(LMS_c[1], _a),
        cam18sl_cone_compress_inverse(LMS_c[2], _a)
    ]


# CAM18sl test with inverse
L_cone, M_cone, S_cone = 25, 100, 75
print("  LMS = %f, %f, %f" % (L_cone, M_cone, S_cone))

# Surround cone value
bg_cone = 0

# M = colourfulness, Q = brightness, A = sorta luminance, ab = opponent signals
cam = cam18sl(L_cone, M_cone, S_cone, bg_cone)
print(" abQA = %f, %f, %f, %f" % (cam["a"], cam["b"], cam["Q"], cam["A"]))

#Inverse
lms2 = cam18sl_inverse(bg_cone, cam["a"], cam["b"], None, cam["Q"])
print("  LMS = %f, %f, %f" % (lms2[0], lms2[1], lms2[2]))
