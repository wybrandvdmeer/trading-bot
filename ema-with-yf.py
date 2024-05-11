import  yfinance as yf

df = yf.download("PANW", period="30d")

df['EMA12'] = df.Close.ewm(span=12).mean()
df['EMA26'] = df.Close.ewm(span=26).mean()
# df['EMA20'] = df.Close.ewm(span=20).mean()
df['EMA20'] = df.Close.ewm(span=20,adjust=True,ignore_na=False).mean()
df['MACD'] = df.EMA12 - df.EMA26

print(df['EMA20'])

