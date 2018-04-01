/*
* (C) 2015,2016 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include "fuzzers.h"
#include <botan/tls_client.h>

class Fuzzer_TLS_Client_Creds : public Botan::Credentials_Manager
   {
   public:
      std::string psk_identity_hint(const std::string&, const std::string&) override { return "psk_hint"; }
      std::string psk_identity(const std::string&, const std::string&, const std::string&) override { return "psk_id"; }
      Botan::SymmetricKey psk(const std::string&, const std::string&, const std::string&) override
         {
         return Botan::SymmetricKey("AABBCCDDEEFF00112233445566778899");
         }
   };

class Fuzzer_TLS_Client_Callbacks : public Botan::TLS::Callbacks
   {
   public:
       void tls_emit_data(const uint8_t[], size_t) override
         {
         // discard
         }

      void tls_record_received(uint64_t, const uint8_t[], size_t) override
         {
         // ignore peer data
         }

      void tls_alert(Botan::TLS::Alert) override
         {
         // ignore alert
         }

      bool tls_session_established(const Botan::TLS::Session&)
         {
         return true; // cache it
         }

      void tls_verify_cert_chain(
         const std::vector<Botan::X509_Certificate>& cert_chain,
         const std::vector<std::shared_ptr<const Botan::OCSP::Response>>& ocsp_responses,
         const std::vector<Botan::Certificate_Store*>& trusted_roots,
         Botan::Usage_Type usage,
         const std::string& hostname,
         const Botan::TLS::Policy& policy) override
         {
         try
            {
            // try to validate to exercise those code paths
            Botan::TLS::Callbacks::tls_verify_cert_chain(cert_chain, ocsp_responses,
                                                         trusted_roots, usage, hostname, policy);
            }
         catch(...)
            {
            // ignore validation result
            }
         }

   };

void fuzz(const uint8_t in[], size_t len)
   {
   if(len == 0)
      return;

   Botan::TLS::Session_Manager_Noop session_manager;
   Botan::TLS::Policy policy;
   Botan::TLS::Protocol_Version client_offer = Botan::TLS::Protocol_Version::TLS_V12;
   Botan::TLS::Server_Information info("server.name", 443);
   Fuzzer_TLS_Client_Callbacks callbacks;
   Fuzzer_TLS_Client_Creds creds;

   Botan::TLS::Client client(callbacks,
                             session_manager,
                             creds,
                             policy,
                             fuzzer_rng(),
                             info,
                             client_offer);

   try
      {
      client.received_data(in, len);
      }
   catch(std::exception& e)
      {
      }

   }

