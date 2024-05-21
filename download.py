import yfinance as yf

class Downloader:
    def download(self, name):
        data = yf.download(name, period="2880m", interval="1m", prepost=True)
        d = data.to_dict('index')
        for i in d.items():
            print(i[0])
            print(i[1]['Close'])

    def download2(self, name):
        ticker = yf.Ticker(name)
        print(ticker.info)

if __name__ == "__main__":
    d = Downloader()
    d.download("ACIW")
