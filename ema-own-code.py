import numpy as np

def exponential_moving_average(prices, period, weighting_factor):
    ema = np.zeros(len(prices))
    sma = np.mean(prices[:period])
    ema[period - 1] = sma
    for i in range(0, len(prices)):
        ema[i] = (prices[i] * weighting_factor) + (ema[i - 1] * (1 - weighting_factor))
        print(ema[i])
    return ema


prices = [272.1499938964844,
          275.0199890136719,
          277.3299865722656,
          281.1400146484375,
          277.7099914550781,
          281.67999267578125,
          293.489990234375,
          293.82000732421875,
          288.7900085449219,
          291.4200134277344,
          293.4100036621094,
          290.8900146484375,
          287.3399963378906,
          295.32000732421875,
          296.2099914550781,
          300.57000732421875,
          305.5199890136719,
          303.010009765625,
          295.6700134277344,
          297.4700012207031]
period = 20
weighting_factor = 2 / 21

ema = exponential_moving_average(prices, period, weighting_factor)
