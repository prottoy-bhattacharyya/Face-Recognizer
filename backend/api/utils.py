import pymysql
from dotenv import load_dotenv
import os

load_dotenv()

def connect_db():
    timeout = 10

    connection = pymysql.connect(
        charset="utf8mb4",
        connect_timeout=timeout,
        cursorclass=pymysql.cursors.DictCursor,
        db=os.getenv("MYSQL_DB"),
        host=os.getenv("MYSQL_HOST"),
        password=os.getenv("MYSQL_PASSWORD"),
        read_timeout=timeout,
        port=int(os.getenv("MYSQL_PORT")),
        user=os.getenv("MYSQL_USER"),
        write_timeout=timeout,
    )

    return connection