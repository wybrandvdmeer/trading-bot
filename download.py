import yfinance as yf

class Downloader:
    def download(self, name):
        data = yf.download(name, period="1d", interval="1m")
        d = data.to_dict('index')
        for i in d.items():
            print(i[0])
            print(i[1]['Close'])

    def download2(self, name):
        ticker = yf.Ticker(name)
        print(ticker.info)

if __name__ == "__main__":
    d = Downloader()
    d.download2("ACIW")
