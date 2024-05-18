import sqlite3
import pandas as pd
import sys
import finplot as fplt
from datetime import datetime, timedelta

ticker = sys.argv[1].upper()

days = 0
if len(sys.argv) > 2:
    days = int(sys.argv[2])

dt = datetime.today() - timedelta(days=days)
db_file = 'file:/db-files/' + ticker + '-' + dt.strftime('%Y%m%d') + '.db?mode=ro'
print("DB-file: " + db_file)
conn = sqlite3.connect(db_file, uri=True)

csr = conn.cursor()
csr.execute("SELECT time, open, close, high, low FROM candles WHERE ticker = '" + ticker + "'")

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

if len(open) == 0:
    print("No data.")
    exit(0)

stock_prices = pd.DataFrame({'open': open , 'close': close , 'high': high,  'low': low },
                            index=pd.date_range("2024-05-14", periods=len(open), freq="d"))

buy_time = []
sell_time = []

csr.execute("SELECT buy_time, sell_time FROM positions WHERE ticker = '" + ticker + "'")

for row in csr:
    buy_time.append(row[0])
    sell_time.append(row[1])

conn.close()

positions = pd.DataFrame({'buy_time': buy_time , 'sell_time': sell_time })
                            #index=pd.date_range("2024-05-14", periods=len(open), freq="d"))

print(stock_prices)
print(positions)

fplt.candlestick_ochl(stock_prices[['open', 'close', 'high', 'low']])
fplt.show()
