import cv2
from django.apps import AppConfig
import os
import requests
from ultralytics import YOLO
from django.conf import settings

from api.utils import connect_db

class ApiConfig(AppConfig):
    name = 'api'
    verbose_name = 'Face Recognition API'
    # download yolov8n-face.pt if it doesn't exist

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.ready()  # Call the ready method to ensure the model is downloaded

    def test_camera(self):
        cap = cv2.VideoCapture(settings.CAMERA_STREAM_URL)
        if not cap.isOpened():
            print("Warning: Could not open camera. Please check your camera settings.")
        else:
            print("Camera is accessible.")
        cap.release()

    def test_yolo_model(self):
        model_path = os.path.join("models", 'yolov8n-face.pt')
        if not os.path.exists(model_path):
            print("Warning: YOLO model file does not exist. Please ensure it is downloaded.")
            return 
        try:
            yolo_model = YOLO(model_path)
            print("YOLO model loaded successfully.")
        except Exception as e:
            print(f"Error loading YOLO model: {e}")

    def download_yolo_model(self):
        model_path = os.path.join("models", 'yolov8n-face.pt')
        face_model_url = "https://github.com/YapaLab/yolo-face/releases/download/1.0.0/yolov8n-face.pt"
        if not os.path.exists(model_path):
            print("Downloading yolov8n-face.pt model...")
            response = requests.get(face_model_url, stream=True)
            with open(model_path, 'wb') as f:
                for chunk in response.iter_content(chunk_size=8192):
                    f.write(chunk)

            print("yolov8n-face.pt model downloaded and saved.")
        else:
            print("yolov8n-face.pt model already exists.")

    def create_tables(self):
        cursor = connect_db().cursor()
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS users (
                id INT AUTO_INCREMENT PRIMARY KEY,
                username VARCHAR(255) UNIQUE NOT NULL,
                password VARCHAR(255) NOT NULL
            )
        """)
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS face_database (
                id INT AUTO_INCREMENT PRIMARY KEY,
                username VARCHAR(255) NOT NULL,
                image_path VARCHAR(255) NOT NULL,
                FOREIGN KEY (username) REFERENCES users(username)
            )
        """)
        connect_db().commit()
        print(
            "Database tables created successfully"
        )

    def ready(self):
        os.makedirs("models", exist_ok=True)
        self.download_yolo_model()
        self.test_camera()
        self.test_yolo_model()
        self.create_tables()
