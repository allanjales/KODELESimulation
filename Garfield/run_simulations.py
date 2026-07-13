#!/usr/bin/env python3
import argparse
import subprocess
import os
from concurrent.futures import ProcessPoolExecutor, as_completed
from tqdm import tqdm

# !--------------------------------------------------------
# ! Worker Function
# !--------------------------------------------------------
def run_simulation(electric_field, executable, n_simulations, gas_file):
	"""
	Runs the C++ simulation for a specific electric field and gas file.
	"""
	cmd = [executable, str(electric_field), str(n_simulations), gas_file]
	
	# Run the process and capture output so it doesn't break the tqdm bar in the terminal
	result = subprocess.run(cmd, capture_output=True, text=True)
	
	# Return the field, success status, and error log (if any)
	success = (result.returncode == 0)
	return electric_field, success, result.stderr

if __name__ == "__main__":
	# !--------------------------------------------------------
	# ! Argument Parsing
	# !--------------------------------------------------------
	parser = argparse.ArgumentParser(description="Run Garfield++ KODELE simulations in parallel.")
	parser.add_argument("-g", "--gas",         type=str,default="cms_rpc_95.2_4.5_0.3_25-40kV.gas",help="Path to the gas file (default: cms_rpc_95.2_4.5_0.3_25-40kV.gas)")
	parser.add_argument("-n", "--nsimulations",type=int,default=1000,help="Number of simulations per electric field (default: 1000)")
	parser.add_argument("-j", "--jobs",        type=int,default=os.cpu_count(),help="Number of parallel workers / CPU cores (default: all available cores)")
	parser.add_argument("-e", "--exec",        type=str,default="./build/NSimulations",dest="executable",help="Path to the compiled C++ executable (default: ./build/NSimulations)")
	
	args = parser.parse_args()

	# !--------------------------------------------------------
	# ! Validation
	# !--------------------------------------------------------
	if not os.path.isfile(args.executable):
		print(f"Error: Executable '{args.executable}' not found. Please compile your C++ code first.")
		exit(1)

	if not os.path.isfile(args.gas):
		print(f"Error: Gas file '{args.gas}' not found.")
		exit(1)

	# Generate electric fields from 4.2 to 5.5 in steps of 0.1
	# Using round() prevents floating-point issues like 4.300000000000001
	electric_fields = [round(4.2 + i * 0.1, 1) for i in range(14)]

	print(f"Starting parallel simulations on {args.jobs} CPU cores...")
	print(f"Executable:          {args.executable}")
	print(f"Gas file:            {args.gas}")
	print(f"Simulations / field: {args.nsimulations}")
	print(f"Electric fields:     {electric_fields}\n")

	errors = []

	# !--------------------------------------------------------
	# ! Parallel Execution
	# !--------------------------------------------------------
	with ProcessPoolExecutor(max_workers=args.jobs) as executor:
		# Create a dictionary mapping the future to its electric field
		future_to_ef = {
			executor.submit(run_simulation, ef, args.executable, args.nsimulations, args.gas): ef 
			for ef in electric_fields
		}

		# Initialize tqdm progress bar
		with tqdm(total=len(electric_fields), desc="Overall Progress", unit="field") as pbar:
			for future in as_completed(future_to_ef):
				ef = future_to_ef[future]
				try:
					field, success, stderr = future.result()
					if not success:
						errors.append((field, stderr))
				except Exception as exc:
					errors.append((ef, str(exc)))
				finally:
					# Update progress bar when a task finishes
					pbar.update(1)

	# !--------------------------------------------------------
	# ! Summary Report
	# !--------------------------------------------------------
	print("\nSimulations completed!")
	if errors:
		print(f"\nWARNING: {len(errors)} simulation(s) failed:")
		for field, err_msg in errors:
			print(f"--- Error for E = {field} kV/mm ---")
			print(err_msg)
	else:
		print("All electric fields were simulated successfully without errors.")