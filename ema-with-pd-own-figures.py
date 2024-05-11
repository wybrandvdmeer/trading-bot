import pandas as pd

prices = [0, 1, 2, 3, 4, 5]

df = pd.DataFrame({'col': prices})

print(df)

a = df['col'].ewm(span=2, min_periods=0, adjust=False, ignore_na=False).mean()
print(a)
