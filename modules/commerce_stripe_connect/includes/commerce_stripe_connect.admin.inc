<?php

/**
 * @file
 * Administrative page callbacks for Commerce Stripe Connect.
 */

/**
 * Form callback: builds the Stripe Connect platform configuration form.
 */
function commerce_stripe_platform_configuration_form(array $form, array &$form_state) {
  $settings = commerce_stripe_connect_get_settings();

  // Build an options list of all enabled currencies.
  // @todo Eventually we should restrict this to currencies supported by Stripe.
  $currency_options = array();

  foreach (commerce_currencies(TRUE) as $currency_code => $currency) {
    $currency_options[$currency_code] = $currency['name'];
  }

  $form['platform_client_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Stripe platform client ID'),
    '#description' => t('Find your client ID at https://dashboard.stripe.com/account/applications/settings.'),
    '#default_value' => $settings['platform_client_id'],
    '#required' => TRUE,
  );
  $form['platform_secret_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Stripe platform account secret key'),
    '#description' => t('Find your secret key at https://dashboard.stripe.com/account/apikeys.'),
    '#default_value' => $settings['platform_secret_key'],
    '#required' => TRUE,
  );

  /**
   * @todo Re-enable this when webhook support is actually added.
   * @see https://www.drupal.org/project/commerce_stripe/issues/2958923
  $form['webhook_signing_secret'] = array(
    '#type' => 'textfield',
    '#title' => t('Webhook signing secret'),
    '#description' => t('Connect application endpoint secret. Get your secret from https://dashboard.stripe.com/account/webhooks'),
    '#default_value' => $settings['webhook_signing_secret'],
  );
   */

  $form['application_fee'] = array(
    '#type' => 'fieldset',
    '#title' => t('Application fee'),
    '#collapsible' => FALSE,
  );
  $form['application_fee']['application_fee_method'] = array(
    '#type' => 'radios',
    '#title' => t('Fee method'),
    '#options' => array(
      'percentage' => t('Percentage'),
      'fixed' => t('Fixed'),
    ),
    '#default_value' => $settings['application_fee_method'],
  );
  $form['application_fee']['application_fee_percentage'] = array(
    '#type' => 'textfield',
    '#title' => t('Fee percentage'),
    '#field_suffix' => '%',
    '#default_value' => $settings['application_fee_percentage'],
    '#size' => 4,
    '#states' => array(
      'visible' => array(
        ':input[name="application_fee_method"]' => array('value' => 'percentage'),
      ),
      'required' => array(
        ':input[name="application_fee_method"]' => array('value' => 'percentage'),
      ),
    ),
  );
  $form['application_fee']['application_fee_amount'] = array(
    '#type' => 'textfield',
    '#title' => t('Fee amount'),
    '#default_value' => $settings['application_fee_amount'],
    '#size' => 4,
    '#states' => array(
      'visible' => array(
        ':input[name="application_fee_method"]' => array('value' => 'fixed'),
      ),
      'required' => array(
        ':input[name="application_fee_method"]' => array('value' => 'fixed'),
      ),
    ),  
  );

  $form['application_fee']['platform_currency'] = array(
    '#type' => 'select',
    '#title' => t('Platform currency'),
    '#description' => t('Get your default currency from https://dashboard.stripe.com/account/payouts'),
    '#options' => $currency_options,
    '#default_value' => $settings['platform_currency'], // !empty($settings['platform_currency']) ? $settings['platform_currency'] : key($currency_options),
    '#states' => array(
      'visible' => array(
        ':input[name="application_fee_method"]' => array('value' => 'fixed'),
      ),
      'required' => array(
        ':input[name="application_fee_method"]' => array('value' => 'fixed'),
      ),
    ),
  );

  // Save a copy of the submit array to reuse but add the other system settings
  // form features from the helper function.
  $submit = !empty($form['#submit']) ? $form['#submit'] : array();
  $submit[] = 'commerce_stripe_platform_configuration_form_submit';

  $form = system_settings_form($form);
  $form['#submit'] = $submit;

  return $form;
}

/**
 * Form callback: validates the Stripe Connect platform configuration form.
 */
function commerce_stripe_platform_configuration_form_validate($form, &$form_state) {
  // If the application fee is calculated via a percentage of the transaction
  // total, ensure we were given a legitimate percentage amount value.
  if ($form_state['values']['application_fee_method'] == 'percentage') {
    $fee_percentage = $form_state['values']['application_fee_percentage'];
    if (!is_numeric($fee_percentage) || ($fee_percentage < 0) || ($fee_percentage >= 100)) {
      form_set_error('application_fee_percentage', t('The fee percentage must be a numeric value between 0 and 100.'));
    }
  }
  
  // If the application fee is calculated as a fixed amount, ensure we were
  // given a non-negative number.
  if ($form_state['values']['application_fee_method'] == 'fixed') {
    $fee_amount =  $form_state['values']['application_fee_amount'];
    if (!is_numeric($fee_amount) || ($fee_amount < 0)) {
      form_set_error('application_fee_amount', t('The fee amount must be a non-negative number.'));
    }
  }
}

/**
 * Form callback: submits the Stripe connect platform configuration form.
 */
function commerce_stripe_platform_configuration_form_submit(array $form, array &$form_state) {
  form_state_values_clean($form_state);

  // @todo Consider changing this to save all values at once for fewer queries.
  foreach ($form_state['values'] as $key => $value) {
    commerce_stripe_connect_set_setting($key, $value);
  }

  drupal_set_message(t('The configuration options have been saved.'));
}

/**
 * Builds the Stripe Connect account admin page.
 */
function commerce_stripe_connect_admin_page() {
  $settings = commerce_stripe_connect_get_settings();

  // Verify required platform settings.
  if (empty($settings['platform_client_id']) || empty($settings['platform_secret_key'])) {
    $content = t('You cannot connect this site to a Standard Stripe account until the platform Client ID is configured.');

    if (user_access('administer commerce stripe platform')) {
      $content .= ' ' . l(t('Configure it now'), 'admin/commerce/config/stripe-platform');
    }

    return $content;
  }

  // Display the Stripe connection status.
  $content = array();

  $content['message'] = array(
    '#type' => 'item',
    '#title' => t('Connection status'),
  );

  // If the site is already connected to a Stripe account, display a form
  // letting the administrator disconnect it.
  if (!empty($settings['connected_account_id'])) {
    // @todo Add a list here of payment method rules configured to use it.
    $content['message']['#markup'] = t('Your site is connected to a Standard Stripe account.');
    $content['disconnect_form'] = drupal_get_form('commerce_stripe_connect_admin_disconnect_form');
  }
  else {
    $content['message']['#markup'] = t('Your site is not currently connected to your Standard Stripe account.');

    $url = 'https://connect.stripe.com/oauth/authorize';

    $query = array(
      'response_type' => 'code',
      'client_id' => $settings['platform_client_id'],
      'scope' => 'read_write',
      'state' => drupal_get_token(),
      'redirect_uri' => url('stripe-connect/oauth', array('absolute' => TRUE)),
    );

    $content['stripe_button'] = array(
      '#type' => 'markup',
      '#markup' => '<a href="' . url($url, array('query' => $query)) . '" class="stripe-connect"><span>' . t('Connect with Stripe') . '</span></a>',
      '#attached' => array(
        'css' => array(
          drupal_get_path('module', 'commerce_stripe_connect') . '/theme/commerce_stripe_connect.admin.css',
        ),
      ),
    );
  }

  return $content;
}

/**
 * Form callback: builds the form to disconnect the site's Stripe account.
 */
function commerce_stripe_connect_admin_disconnect_form(array $form, array &$form_state) {
  $form['disconnect'] = array(
    '#type' => 'submit',
    '#value' => t('Disconnect now'),
  );

  return $form;
}

/**
 * Form callback: submits the form to disconnect the site's Stripe account.
 */
function commerce_stripe_connect_admin_disconnect_form_submit($form, &$form_state) {
  // Exit now if for some reason the Stripe library is unavailable.
  if (!commerce_stripe_load_library()) {
    drupal_set_message(t('The form did not submit because the Stripe PHP library was not found.'), 'error');
    return;
  }

  // Load the Stripe Connect settings and trim the API key if necessary.
  $settings = commerce_stripe_connect_get_settings();
  $settings['platform_secret_key'] = trim($settings['platform_secret_key']);

  try {
    // Verify that account has not already been disconnected.
    \Stripe\Stripe::setApiKey($settings['platform_secret_key']);
    $account = \Stripe\Account::retrieve($settings['connected_account_id']);

    // Disconnect account.
    $ch = curl_init();
    curl_setopt_array($ch, array(
      CURLOPT_URL => 'https://connect.stripe.com/oauth/deauthorize',
      CURLOPT_RETURNTRANSFER => true,
      CURLOPT_HTTPHEADER => array('Authorization: Bearer ' . $settings['platform_secret_key']),
      CURLOPT_POST => true,
      CURLOPT_POSTFIELDS => http_build_query(array(
        'client_id' => $settings['platform_client_id'],
        'stripe_user_id' => $settings['connected_account_id'],
      ))
    ));
    $response = curl_exec($ch);
    $response_body = drupal_json_decode($response);

    // Display any errors if found.
    if (!empty($response_body['error'])) {
      drupal_set_message(check_plain($response_body['error_description']), 'error');
      return;
    }

    // Show a 404 page if we received malformed data in the response.
    if (empty($response_body['stripe_user_id'])) {
      drupal_not_found();
    }
  }
  catch (Exception $e) {
    // An exception will be thrown if the account could not be loaded from
    // Stripe. We continue submitting this function to finalize the disconnect.
  }

  // Remove connected account id and keys from settings.
  commerce_stripe_connect_set_setting('connected_account_id', '');
  commerce_stripe_connect_set_setting('connected_secret_key', '');
  commerce_stripe_connect_set_setting('connected_public_key', '');

  drupal_set_message('Your Stripe account has been disconnected. Any payment methods that were using it will need to be reconfigured.');
}
