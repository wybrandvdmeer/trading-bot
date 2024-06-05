#!/usr/bin/env python 

import sqlite3
import pandas as pd
from datetime import datetime, timedelta
import argparse
from os import listdir
from os.path import isfile, join
import warnings

warnings.simplefilter(action='ignore', category=FutureWarning)

DB_LOCATION = '/db-files'

parser = argparse.ArgumentParser()
parser.add_argument('--date', type=str)
parser.add_argument('--strategy', type=str)
args = parser.parse_args()

date = args.date
strategy = args.strategy

if date == None:
    date = datetime.today().strftime('%Y%m%d')

db_files = [f for f in listdir(DB_LOCATION) if isfile(join(DB_LOCATION, f))]

positions = pd.DataFrame(columns=['buy_price', 'sell_price', 'no_of_stocks'])

for f in db_files:
    if f.find(date) != -1:
        if strategy is not None and strategy not in f:
            continue
            
        db_file = 'file:/db-files/' + f + '?mode=ro'

        conn = sqlite3.connect(db_file, uri=True)
        csr = conn.cursor()

        csr.execute("SELECT DISTINCT ticker FROM candles")
        rc = csr.fetchone()
        if rc == None:
            continue

        ticker = rc[0]

        print(ticker)

        ids = []
        buy_time = []
        sell_time = []
        buy_price = []
        sell_price = []
        no_of_stocks = []

        csr.execute(
            "SELECT id, buy_time, sell_time, stock_price, sell_price, no_of_stocks FROM positions WHERE ticker = '" + ticker + "' AND sell_price IS NOT NULL")

        for row in csr:
            buy_price.append(row[3])
            sell_price.append(row[4])
            no_of_stocks.append(row[5])

        p2 = pd.DataFrame(columns=['buy_price', 'sell_price', 'no_of_stocks'])
        p2['buy_price'] = buy_price
        p2['sell_price'] = sell_price
        p2['no_of_stocks'] = no_of_stocks

        positions = pd.concat([positions, p2])

positions['gain'] = positions['no_of_stocks'] * (positions['sell_price'] - positions['buy_price'])
gain = positions['gain'].sum()
no_of_stocks = positions['no_of_stocks'].sum()
no_of_trades = len(positions.index)

print(date + ': gain: ' + str(gain) + ', no_of_stocks: ' + str(no_of_stocks) + ', no_of_trades: ' + str(no_of_trades))
