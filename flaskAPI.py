from flask import Flask, request, jsonify
import joblib
import pandas as pd


model = joblib.load('final_model.pkl')

app = Flask(__name__)

@app.route('/')
def home():
    return "Welcome to the Logistic Regression Model Prediction API!"

@app.route('/predict', methods=['POST'])
def predict():
    try:
        data = request.get_json(force=True)
        print(data) 
        # S'assurer que les clés correspondent à ce que le modèle attend
        data_df = pd.DataFrame([data], columns=["HR (BPM)", "SpO2 (%)", "TEMP (*C)"])
        
        # Vérifier que toutes les colonnes nécessaires sont présentes
        if data_df.isnull().any().any():
            return jsonify({'error': 'Missing data for required fields'}), 400

        predictions = model.predict(data_df)
        print({'predictions': predictions.tolist()})
        return jsonify({'predictions': predictions.tolist()})
    except Exception as e:
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5002)
