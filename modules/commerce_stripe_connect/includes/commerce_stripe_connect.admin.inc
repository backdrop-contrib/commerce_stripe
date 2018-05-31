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

  // If the site is already connected to a Stripe account, display a message.
  if (!empty($settings['connected_account_id'])) {
    return t('Your site is already connected to a Stripe account.');
  }

  // Otherwise display a Stripe Connect button.
  $content = array();

  $content['message'] = array(
    '#type' => 'item',
    '#title' => t('Connection status'),
    '#markup' => t('Your site is not currently connected to your Standard Stripe account.'),
  );

  $url = 'https://connect.stripe.com/oauth/authorize?response_type=code&client_id=' . $settings['platform_client_id'] . '&scope=read_write&state=' . drupal_get_token();

  $content['stripe_button'] = array(
    '#type' => 'markup',
    '#markup' => '<a href="' . $url . '" class="stripe-connect"><span>' . t('Connect with Stripe') . '</span></a>',
    '#attached' => array(
      'css' => array(
        drupal_get_path('module', 'commerce_stripe_connect') . '/theme/commerce_stripe_connect.admin.css',
      ),
    ),
  );

  return $content;
}
