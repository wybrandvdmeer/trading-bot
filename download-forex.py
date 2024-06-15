import datetime as dt
import yfinance as yf

start_date = dt.datetime.today()- dt.timedelta(4000)
end_date = dt.datetime.today()
stock ="USDJPY=X"
data = yf.download(stock, period="1d", interval="1m")
print(data)