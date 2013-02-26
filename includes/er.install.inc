<?php

function install_all(){
	//Set up some default values; these are used on the summary table. see er.admin.inc
	variable_set('er_summary_types', array());//
	variable_set('er_start_date', array('month'=>'9', 'day'=>'15', 'year'=>'2009'));
	
	//TODO: needs to use Batch API
	er_import_taxonomy();
	
	// This section is for Content Types	
// BEGIN BATCH API CODE
	$batch = array (
		'operations' => array(),
		'finished' => 'batch_has_finished',
		'title' => t('Processing ER Core Bundles'),
		'init_message' => t('ER Core Bundles is starting.'),
		'progress_message' => t('Processed @current out of @total.'),
		'error_message' => t('EPSCoR Bundle Batch has encountered an error.'),
	);
	// This function returns array of files names in the install/bundles directory
	$bundle_names = get_install_filenames('bundles'); // argument is the folder name
	
	// Here's where we do a batch call, one file at a time
	foreach ($bundle_names as $file) {
		// Add a batch operation for each file import
		// 		Translates to: "call the function er_import_fields() with two arguments: the filename and the total"
		$batch['operations'][] = array('er_import_fields', array($file, count($bundle_names)));
	}
	batch_set($batch); // Standard Batch call
	// Send them to this location after it's complete
	// 		If we're doing a bunch of batches, you can leave this argument empty
	batch_process('admin/config/epscor/er/status');	
	
	watchdog('er', 'Finished installing Content Types and User Account fields');
	drupal_set_message('Finished installing Content Types and User Account fields');
// END BATCH API CODE	
	
	//TODO: all three calls below need to use Batch API
	er_import_flags();
	er_import_rules();
	er_import_perms();
}

/*
 * This function returns the list of all bundle filenames
 */
function get_install_filenames($folder){
	// get list of all files in the bundles directory
	$drupal_path = ER_INSTALL_DIR . '/' . $folder;
	$names = array();
	// if (function_exists('bundle_copy_import_submit')){ // double check the dependency worked for bundle copy			
		$dir = new DirectoryIterator($drupal_path);
		// loop through all bundle files in the directory
		foreach ($dir as $fileinfo) {
			if (!$fileinfo->isDot()) {
				$names[] = $fileinfo->getFilename();			
			}
		}
	// }
	// else{
		// drupal_set_message('No bundle_copy_import_submit function found in er.install');
		// watchdog('er', 'No bundle_copy_import_submit function found in er.install', NULL, WATCHDOG_CRITICAL);
	// }
	return $names;	
}

/**
* Batch 'finished' callback
*/
function batch_has_finished($success, $results, $operations) {
  if ($success) {
  	//TODO: why are the $results and $operations not working
    // Here we do something meaningful with the results.
    // d($success, 'success');
    // d($results, 'results');
    // d($operations, 'operations');
    // $message = count($results) .' processed.';
    $message = 'Batch processed.';
    // $message .= theme('item_list', $results);  // D6 syntax
    drupal_set_message($message);
  }
  else {
    // An error occurred.
    // $operations contains the operations that remained unprocessed.
    $error_operation = reset($operations);
    $message = t('An error occurred while processing %error_operation with arguments: @arguments', array('%error_operation' => $error_operation[0], '@arguments' => print_r($error_operation[1], TRUE)));
    drupal_set_message($message, 'error');
  }
 
}

function er_import_fields($file, $total, &$context){
	// BEGIN BATCH API CODE
	if (!isset($context['sandbox']['progress'])) {
		$context['sandbox']['progress'] = 0;
		$context['sandbox']['current_node'] = 0; // don't know if we need this line at all
		$context['sandbox']['max'] = $total;
	}
	// END BATCH API CODE
	
	// TODO: try placing this module_load_include into the .install and see if it works
	// Make sure we have the ctools include file loaded so we can use ctools_var_export()
	module_load_include('inc', 'ctools', 'includes/export');
		
	// TODO: try moving this to the .install too
	$drupal_path = ER_INSTALL_DIR . '/bundles';
	watchdog('er', 'Looping bundle '.$file);
	// after the include, we have access to the $data variable for this object
	include($drupal_path . '/' . $file);
	$form_state = array();
	// 	ctools_var_export() will take an object and turn it into nice text basically. 
	// 		that's what we put into the import textarea
	$ready_for_textarea = '$data = ' . ctools_var_export($data) . ';';
	// d($ready_for_textarea, 'ready for textarea');
	$form_state['values']['macro'] = $ready_for_textarea; // remember $data comes into being from the include call
	// TODO: add guard here making sure the function exists
	bundle_copy_import_submit('', $form_state); // function doesn't use first arg


	// BEGIN BATCH API CODE
	// 		Update the progress for the Batch API
	$context['sandbox']['progress']++;
    // $context['sandbox']['current_node'] = 0; // TODO: I think we can get rid of this
    $context['message'] = t('Now processing bundle %b', array('%b' => $file));	
	if ($context['sandbox']['progress'] == $total){
		// Give the signal that it's finished
		$context['sandbox']['finished'] = 1;
	}
	// END BATCH API CODE
}

/* this function is called from er.admin.inc when they reset individual bundles */
function er_revert_bundle($form, &$form_state){
	d("This function er_revert_bundle is not done. It will be allow the importation of individuals bundles. I am in er.module");
	d($form_state['values'], 'form_state values in er_revert_bundle');
	
	// get list of all the keys with a value of 1, which in this case indicates all the bundles to import
	$bundle_names = array();
	foreach ($form_state['values'] as $key => $value) {
		if ($value == 1){
			$bundle_names[] = $key;
		}			  
	}
	// now we have a list of filenames that exist in the install/bundles directory
// BEGIN BATCH API CODE
	$batch = array (
		'operations' => array(),
		'finished' => 'batch_has_finished',
		'title' => t('Processing ER Core Bundles'),
		'init_message' => t('ER Core Bundles is starting.'),
		'progress_message' => t('Processed @current out of @total.'),
		'error_message' => t('EPSCoR Bundle Batch has encountered an error.'),
	);
	
	// Here's where we do a batch call, one file at a time
	foreach ($bundle_names as $file) {
		// Add a batch operation for each file import
		// 		Translates to: "call the function er_import_fields() with two arguments: the filename and the total"
		$batch['operations'][] = array('er_import_fields', array($file, count($bundle_names)));
	}
	batch_set($batch); // Standard Batch call
	// Send them to this location after it's complete
	// 		If we're doing a bunch of batches, you can leave this argument empty
	batch_process('admin/config/epscor/er/testing'); // this call will be done at the end, for now it's here to demonstrate
	
	watchdog('er', 'Finished updating Content Types');
	drupal_set_message('Finished updating Content Types');
// END BATCH API CODE		
}

/* this function is called from er.admin.inc when they reset fields, rules, flags, etc. */
function er_revert($form, &$form_state){
/* The order of this install is as such:

	- create taxonomy terms, they will be used by Entity Reference fields when we create Content Types
	- create Content Types and Fields, create User Profile Fields
	- create flags, that may now attach to the proper bundles (content types)
	- create rules
*/
	
	if ($form_state['values']['redo_checkboxes']['Taxonomy'] !== 0){ // you need the explicit equals btw
		// Create taxonomy terms
		// d('you checked Taxonomy');
		er_import_taxonomy();	
	}
	
	// This is the one MIKE is assigned to
	if ($form_state['values']['redo_checkboxes']['Fields'] !== 0){
		// Create Content Types and Field Settings, User Profile Fields
		// d('you checked Fields');
		er_import_fields();	
	}	
	
	if ($form_state['values']['redo_checkboxes']['Flags'] !== 0){	
		// Create flags
		// d('you checked Flags');
		er_import_flags();
	}
	
	if ($form_state['values']['redo_checkboxes']['Rules'] !== 0){	
		// Create rules
		// d('you checked Rules');
		er_import_rules();
	}
	
	if ($form_state['values']['redo_checkboxes']['Permissions'] !== 0){	
		// Reset permissions
		// d('you checked Permissions');
		er_import_perms();
		drupal_set_message('Roles and permissions have been reset.');
	}
		
}

/* this function is called from er.admin.inc when they reset the permissions */
function er_import_perms(){
	$er_roles = array('student', 'administrative staff', 'faculty');
	// these are the two dumps from phpMyAdmin, by including them, 
	//   we load the arrays ($role_permission and $role) and can use them
	$perms_path = ER_INSTALL_DIR . '/roles_perms/';
	include $perms_path . 'role.php';
	include $perms_path . 'role_permission.php';

	foreach ($er_roles as $er_role){
		// note: searchRIDdb and searchRIDfile are defined in this same file, not API calls
		if (!is_null($student_rid = searchRIDdb($er_role))){
			// Remove role, this is a Drupal API call 
			// http://api.drupal.org/api/drupal/modules!user!user.module/function/user_role_delete/7
			user_role_delete($er_role);
			watchdog('er', 'Deleted existing role '.$er_role);
		}
		
		// find indexes for permissions involving role
		$ridArr = array();	
		foreach ($role_permission as $row){
			$ridArr[] = $row['rid'];
		}
	
		// find out RID from database
		$ridIdx = searchRIDfile($role, $er_role);
		$rid = $role[$ridIdx]['rid'];
		
		$justIdx = array_keys($ridArr, $rid);
		
		// create role, remember we deleted it if it existed earlier
		$newrole = new stdClass();
		$newrole->name = $er_role;
		user_role_save($newrole);		
		
		// Role to grant the permissions to
		$newrole = user_role_load_by_name($er_role);
		$newrole_rid = $newrole->rid;
		// Define our $er_role role permissions
		$array_perms = array();
		// Grant permissions to our $er_role role		
		// loop through all of those indexes, telling what permissions they are		
		
		foreach ($justIdx as $idx){
			// This next call has to do with avoiding a Fatal Error when you try to add a permission
			//	that's not associated with an enabled module.		
			// see http://drupal.org/node/737816 and http://drupal.org/node/1249952 for more info	
			$modules = user_permission_get_modules();
			if (!is_null($modules[$role_permission[$idx]['permission']])){
				// there is an enabled/recognized module associated with this permission
				$array_perms[] = $role_permission[$idx]['permission'];
			}
		}
		
		// Add that array of permissions to the role. a Drupal API call
		user_role_grant_permissions($newrole_rid, $array_perms);
		watchdog('er', 'Created and granted permissions to role '.$er_role);			
	}	
}

function searchRIDdb($name){
	$result = db_query("SELECT rid, name FROM {role} WHERE name = :name", array(
	  ':name' => $name,
	));
	$rid = $result->fetchAll();
	if (empty($rid)){
		// d($name . ' does not exist.');
		return NULL;
	}		
	else {
		$rid = $rid[0]->rid;
		// d($rid, $name . ' exists as ');
		return $rid;
	}	
}

function searchRIDfile($array, $name) {
   foreach ($array as $key => $val) {
       if ($val['name'] === $name) {
           return $key;
       }
   }
   return null;
}

function er_import_rules(){
	// basically I'll be mimicking functionality in the import function, check to make sure it exists, for good measure
	$ret = module_load_include('inc', 'rules', 'ui/ui.forms'); // otherwise it can't see the file
	 //require_once(drupal_get_path('module', 'rules_admin') .'/ui/ui.forms.inc');
	if (function_exists('rules_ui_import_form_validate')){
		watchdog('er', 'Begin creating rules, found the rules_ui_import_form_validate function');

		$drupal_path = ER_INSTALL_DIR . '/rules';
		$dir = new DirectoryIterator($drupal_path);
		// loop through all rule files in the directory
		foreach ($dir as $fileinfo) {
			if (!$fileinfo->isDot()) {
				$rule_filename = $fileinfo->getFilename();
				watchdog('er', 'Looping rule '.$rule_filename);
				// this grabs the text from the file and puts it in a variable
				$rules_config = file_get_contents($drupal_path . '/' . $rule_filename);
				
				$form = array();
				$form_state = array();
				$form_state['values']['import'] = $rules_config;
				$form_state['values']['overwrite'] = 1;
				
				// for these imports, work gets done in the _validate and _submit functions to do the actual import
				rules_ui_import_form_validate($form, $form_state);
				rules_ui_import_form_submit($form, $form_state);				
			}
		}
	}
	else{		
		watchdog('er', 'No rules_ui_import_form_validate function found in er.install', NULL, WATCHDOG_CRITICAL);
		drupal_set_message('No rules_ui_import_form_validate function found in er.install');
	}	
}

function er_import_flags(){
	// again I'm mimicking functionality in the import function, check to make sure it exists, for good measure
	module_load_include('inc', 'flag', 'includes/flag.export');
	 //require_once(drupal_get_path('module', 'rules_admin') .'/ui/ui.forms.inc');
	if (function_exists('flag_import_form_validate')){
		watchdog('er', 'Begin creating flags, found the flag_import_form_validate function');

		$drupal_path = ER_INSTALL_DIR . '/flags';
		$dir = new DirectoryIterator($drupal_path);
		// loop through all flag files in the directory
		foreach ($dir as $fileinfo) {
			if (!$fileinfo->isDot()) {//Determine if current DirectoryIterator item is '.' or '..'
				$flag_filename = $fileinfo->getFilename();
				watchdog('er', 'Looping flag '.$flag_filename);
				// this grabs the text from the file and puts it in a variable
				$flag_config = file_get_contents($drupal_path . '/' . $flag_filename);
				
				$form = array();
				$form_state = array();
				$form_state['values']['import'] = $flag_config;
				
				// for these imports, work gets done in the _validate and _submit functions to do the actual import
				flag_import_form_validate($form, $form_state);
				flag_import_form_submit($form, $form_state);
			}
		}
	}
	else{		
		watchdog('er', 'No flag_import_form_validate function found in er.install', NULL, WATCHDOG_CRITICAL);
		drupal_set_message('No flag_import_form_validate function found in er.install');
	}		
}

function er_import_fields_OLD(){	
	if (function_exists('bundle_copy_import_submit')){ // double check the dependency worked for bundle copy
	
		$drupal_path = ER_INSTALL_DIR . '/bundles';
		$dir = new DirectoryIterator($drupal_path);
		// loop through all rule files in the directory
		foreach ($dir as $fileinfo) {
			if (!$fileinfo->isDot()) {
				$bundle_filename = $fileinfo->getFilename();
				watchdog('er', 'Looping bundle '.$bundle_filename);
				// this grabs the text from the file and puts it in a variable
				$bundle_config = file_get_contents($drupal_path . '/' . $bundle_filename);
				$form_state = array();
				$form_state['values']['macro'] = $bundle_config;
				bundle_copy_import_submit('', $form_state); // function doesn't use first arg			
			}
		}
	}
	else{
		drupal_set_message('No bundle_copy_import_submit function found in er.install');
		watchdog('er', 'No bundle_copy_import_submit function found in er.install', NULL, WATCHDOG_CRITICAL);
	}
	watchdog('er', 'Finished installing Content Types and User Account fields');
	drupal_set_message('Finished installing Content Types and User Account fields');	
}


function er_import_taxonomy(){
	// create taxonomy term 1 for the Project Compnents
	// make sure we're not uninstalling and reinstalling it
	$vid = db_query("SELECT vid FROM {taxonomy_vocabulary} WHERE machine_name = 'er_components'")->fetchField();
	if (!is_numeric($vid)){
		watchdog('er', 'Creating taxonomy er_components');
		taxonomy_vocabulary_save((object) array(
		  'name' => 'Component',
		  'machine_name' => 'er_components',
		));	
	}
	else{
		watchdog('er', 'NOT creating taxonomy er_components, it already exists');
	}

	// create taxonomy term 2 for the Collaborations
	// again, make sure it's not already installed, otherwise fatal errors
	$vid = db_query("SELECT vid FROM {taxonomy_vocabulary} WHERE machine_name = 'collaborations'")->fetchField();
	if (!is_numeric($vid)){
		watchdog('er', 'Creating taxonomy collaborations');
		taxonomy_vocabulary_save((object) array(
		  'name' => 'Collaborations',
		  'machine_name' => 'collaborations',
		));	
	} else {
		watchdog('er', 'NOT creating taxonomy er_components, it already exists');
	}	
}