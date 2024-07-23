from flask import Flask, request, jsonify
import joblib
import pandas as pd

# Load the trained model
model = joblib.load('./model.pkl')

# Initialize the Flask app
app = Flask(__name__)

@app.route('/')
def home():
    return "Welcome to the model prediction API!"

@app.route('/predict', methods=['POST'])
def predict():
    data = request.get_json(force=True)
    
    # Convert data into DataFrame
    data_df = pd.DataFrame([data])
    expected_columns = [" HR (BPM)", " SpO2 (%)", "TEMP (*C)"]
    data_df = data_df[expected_columns]

    # Preprocess data using the pipeline's imputer
    imputer = model.named_steps['imputer']
    data_imputed = pd.DataFrame(imputer.transform(data_df), columns=expected_columns)
    
    # Predict using the model
    predictions = model.named_steps['model'].predict(data_imputed)
    
    # Return the predictions as a JSON response
    return jsonify({'predictions': predictions.tolist()})

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5002)