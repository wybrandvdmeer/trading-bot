#!/usr/bin/env python 

import sqlite3
import pandas as pd
import finplot as fplt
from datetime import datetime, timedelta
from dateutil.tz import gettz
import argparse

def fill_init_values(open, lst):
    lst = [x for x in lst if x is not None]
    if len(lst) == 0:
        return None
    else:
        init_value = lst[0]

    for i in range(0, len(open) - len(lst)):
        lst.insert(0, init_value)

    return lst

days = 0

parser = argparse.ArgumentParser()
parser.add_argument('--db-file', type=str)
parser.add_argument('--ticker', type=str)
parser.add_argument('--strategy', type=str)
parser.add_argument('--days', type=int)
args = parser.parse_args()

ticker = args.ticker
if ticker is not None:
    ticker = ticker.upper()

strategy = args.strategy
if strategy is None:
    strategy = 'macd'

if args.days != None:
    days = args.days

db_file = args.db_file
dt = datetime.today() - timedelta(days=days)
date_string = dt.strftime('%Y%m%d')
if db_file is None:
    db_file = 'file:/db-files/' + ticker + '-' + date_string + '-' + strategy + '.db?mode=ro'

print("DB-file: " + db_file)
conn = sqlite3.connect(db_file, uri=True)
csr = conn.cursor()

if ticker is None:
    csr.execute("SELECT DISTINCT ticker FROM candles")
    ticker = csr.fetchone()[0]

sql = "SELECT time, open, close, high, low, macd, signal, sma_50, sma_200, custom_ind1, custom_ind2, custom_ind3 FROM candles where ticker = '" + ticker + "' ORDER BY time"
csr.execute(sql)

time = []
open = []
close = []
low = []
high = []
mac_d = []
signal = []
sma_50 = []
sma_200 = []
custom_ind1 = []
custom_ind2 = []
custom_ind3 = []

fplt.display_timezone = gettz('US/Eastern')

ax, ax2 = fplt.create_plot(ticker + '-' + date_string, rows=2)

for row in csr:
    if row[1] == 0:
        continue
    open.append(row[1])
    close.append(row[2])
    high.append(row[3])
    low.append(row[4])
    mac_d.append(row[5])
    signal.append(row[6])
    sma_50.append(row[7])
    sma_200.append(row[8])
    custom_ind1.append(row[9])
    custom_ind2.append(row[10])
    custom_ind3.append(row[11])
    time.append(datetime.utcfromtimestamp(row[0]))

if len(open) == 0:
    print("No data.")
    exit(0)

custom_ind1 = fill_init_values(open, custom_ind1)
custom_ind2 = fill_init_values(open, custom_ind2)
custom_ind3 = fill_init_values(open, custom_ind3)

stock_prices = pd.DataFrame({'datetime': time, 'open': open, 'close': close, 'high': high, 'low': low})
stock_prices = stock_prices.set_index('datetime')

max_open = stock_prices['open'].max()
min_open = stock_prices['open'].min()
max_open -= ((max_open - min_open) / 4)

dates = []
for d in stock_prices.index:
    dates.append(d)

ids = []
buy_time = []
sell_time = []
buy_price = []
sell_price = []
no_of_stocks = []

csr.execute(
    "SELECT id, buy_time, sell_time, stock_price, sell_price, no_of_stocks FROM positions WHERE ticker = '" + ticker + "' AND sell_price IS NOT NULL")

for row in csr:
    ids.append(row[0])
    buy_time.append(datetime.utcfromtimestamp(row[1]))
    sell_time.append(datetime.utcfromtimestamp(row[2]))
    buy_price.append(row[3])
    sell_price.append(row[4])
    no_of_stocks.append(row[5])

conn.close()

positions = pd.DataFrame(
    {'ids': ids, 'buy_time': buy_time, 'sell_time': sell_time, 'buy_price': buy_price,
     'sell_price': sell_price, 'no_of_stocks': no_of_stocks})
positions['gain'] = positions['no_of_stocks'] * (positions['sell_price'] - positions['buy_price'])
gain = positions['gain'].sum()
no_of_stocks = positions['no_of_stocks'].sum()
no_of_trades = len(positions.index)
positions.reset_index()

fplt.plot(mac_d, ax=ax2, legend='MACD')
fplt.plot(signal, ax=ax2, legend='Signal')
fplt.candlestick_ochl(stock_prices[['open', 'close', 'high', 'low']])
fplt.plot(sma_50, ax=ax, legend='SMA-50')
fplt.plot(sma_200, ax=ax, legend='SMA-200')
if custom_ind1 != None:
    fplt.plot(custom_ind1, ax=ax, legend='custom_ind1')
if custom_ind2 != None:
    fplt.plot(custom_ind2, ax=ax, legend='custom_ind2')
if custom_ind3 != None:
    fplt.plot(custom_ind3, ax=ax, legend='custom_ind3')

print(positions)

txt = "Gain: " + str(gain) + '\nTotal no of stocks: ' + str(no_of_stocks) + '\nTotal no of trades: ' + str(no_of_trades)
print(txt)
fplt.add_text((time[10], max_open), txt, color='#bb7700', ax=ax)

for index, row in positions.iterrows():
    id = row['ids']
    x1 = row['buy_time']
    y1 = row['buy_price']
    x2 = row['sell_time']
    y2 = row['sell_price']

    line = fplt.add_line((x1, y1), (x2, y2), color='#9900ff', interactive=True)

    txt = 'id: ' + str(id) + ' buy: ' + str(y1) + ' sell: ' + str(y2) + ' gain/stock: ' + "{:.2f}".format(y2 - y1)
    fplt.add_text((x1, y1), txt, color='#bd7700', ax=ax)

conn.close()

fplt.show()
