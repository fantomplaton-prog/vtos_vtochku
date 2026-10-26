import numpy as np
import json


def npz_to_json():
    data = np.load('data_coll_params.npz', allow_pickle=True)
    json_dict = {}
    for key in data.files:
        array = data[key]
        if isinstance(array, np.ndarray):
            json_dict[key] = array.tolist()
        else:
            json_dict[key] = array.item() if hasattr(array, 'item') else array
    with open('data_passport.json', 'w') as f:
        json.dump(json_dict, f, indent=4)


if __name__ == '__main__':
    npz_to_json()
