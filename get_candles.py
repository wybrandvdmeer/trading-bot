import sqlite3
import pandas as pd
import sys
import finplot as fplt
from datetime import datetime, timedelta
from dateutil.tz import gettz

ticker = sys.argv[1].upper()

days = 0
if len(sys.argv) > 2:
    days = int(sys.argv[2])

dt = datetime.today() - timedelta(days=days)
db_file = 'file:/db-files/' + ticker + '-' + dt.strftime('%Y%m%d') + '.db?mode=ro'
print("DB-file: " + db_file)
conn = sqlite3.connect(db_file, uri=True)

csr = conn.cursor()
csr.execute("SELECT time, open, close, high, low, macd, signal, sma_200 FROM candles WHERE ticker = '" + ticker + "'")

time = []
open = []
close = []
low = []
high = []
mac_d = []
signal = []
sma_200 = []

fplt.display_timezone = gettz('US/Eastern')

ax, ax2 = fplt.create_plot('S&P 500 MACD', rows=2)

for row in csr:
    if row[1] == 0:
        continue
    open.append(row[1])
    close.append(row[2])
    high.append(row[3])
    low.append(row[4])
    mac_d.append(row[5])
    signal.append(row[6])
    sma_200.append(row[7])
    time.append(datetime.utcfromtimestamp(row[0]))

if len(open) == 0:
    print("No data.")
    exit(0)

stock_prices = pd.DataFrame({'datetime': time, 'open': open, 'close': close, 'high': high, 'low': low})
stock_prices = stock_prices.set_index('datetime')

dates = []
for d in stock_prices.index:
    dates.append(d)

buy_time = []
sell_time = []
buy_price = []
sell_price = []

csr.execute("SELECT buy_time, sell_time, stock_price, sell_price FROM positions WHERE ticker = '" + ticker + "'")

for row in csr:
    buy_time.append(datetime.utcfromtimestamp(row[0]))
    sell_time.append(datetime.utcfromtimestamp(row[1]))
    buy_price.append(row[2])
    sell_price.append(row[3])

conn.close()

fplt.plot(mac_d, ax=ax2, legend='MACD')
fplt.plot(signal, ax=ax2, legend='Signal')
fplt.candlestick_ochl(stock_prices[['open', 'close', 'high', 'low']])

positions = pd.DataFrame({'buy_time': buy_time, 'sell_time': sell_time, 'buy_price': buy_price, 'sell_price': sell_price})
positions.reset_index()

for index, row in positions.iterrows():
    x1 = row['buy_time']
    y1 = row['buy_price']
    x2 = row['sell_time']
    y2 = row['sell_price']

    line = fplt.add_line((x1, y1), (x2, y2), color='#9900ff', interactive=True)

conn.close()

fplt.show()
