import sqlite3
import pandas as pd

import finplot as fplt

conn = sqlite3.connect('/tmp/tb.db')

csr = conn.cursor()
csr.execute("SELECT time, open, close, high, low FROM candles WHERE ticker = 'BTDPF'")

time = []
open = []
close = []
low = []
high = []

for row in csr:
    if row[1] == 0:
        continue
    time.append(row[0])
    open.append(row[1])
    close.append(row[2])
    high.append(row[3])
    low.append(row[4])
conn.close()

if len(open) == 0:
    print("No data.")
    exit(0)

stock_prices = pd.DataFrame({'open': open , 'close': close , 'high': high,  'low': low },
                            index=pd.date_range("2024-05-14", periods=len(open), freq="d"))

print(stock_prices)
fplt.candlestick_ochl(stock_prices[['open', 'close', 'high', 'low']])
fplt.show()